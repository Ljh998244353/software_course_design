const state = {
    data: null,
    activeAccountIndex: 0,
};

const statusText = {
    RUNNING: "竞拍中",
    PENDING_START: "待开始",
    SOLD: "已成交",
    UNSOLD: "已流拍",
    REVIEWED: "已评价",
    SUCCESS: "成功",
    WINNING: "领先",
    OUTBID: "被超越",
};

const summaryMap = [
    ["demoUserCount", "演示账号"],
    ["demoItemCount", "演示拍品"],
    ["demoAuctionCount", "拍卖活动"],
    ["demoBidCount", "出价记录"],
    ["demoOrderCount", "演示订单"],
    ["demoGmvAmount", "成交金额"],
];

function $(selector) {
    return document.querySelector(selector);
}

function setText(selector, text) {
    const element = $(selector);
    if (element) {
        element.textContent = text;
    }
}

function safe(value, fallback = "暂无") {
    if (value === null || value === undefined || value === "") {
        return fallback;
    }
    return value;
}

function statusLabel(value) {
    return statusText[value] || value || "未知";
}

function money(value) {
    const parsed = Number(String(value || "0").replace(/,/g, ""));
    if (Number.isNaN(parsed)) {
        return value || "0.00";
    }
    return parsed.toLocaleString("zh-CN", {
        minimumFractionDigits: 2,
        maximumFractionDigits: 2,
    });
}

function toast(message) {
    const element = $("#toast");
    element.textContent = message;
    element.classList.add("show");
    window.clearTimeout(toast.timer);
    toast.timer = window.setTimeout(() => element.classList.remove("show"), 3200);
}

function renderSkeletons() {
    $("#summaryCards").innerHTML = '<div class="skeleton"></div><div class="skeleton"></div><div class="skeleton"></div><div class="skeleton"></div>';
    $("#auctionGrid").innerHTML = '<div class="skeleton"></div><div class="skeleton"></div><div class="skeleton"></div><div class="skeleton"></div>';
    $("#roleTabs").innerHTML = '<div class="skeleton"></div>';
    $("#roleCard").innerHTML = '<div class="skeleton"></div>';
}

async function loadDashboard() {
    renderSkeletons();
    try {
        const response = await fetch("/api/demo/dashboard", {
            headers: { Accept: "application/json" },
        });
        const payload = await response.json();
        if (!response.ok || payload.code !== 0) {
            throw new Error(payload.message || `接口返回 ${response.status}`);
        }
        state.data = payload.data;
        renderDashboard(payload.data);
        toast("演示数据已从 MySQL 刷新");
    } catch (error) {
        showLoadFailure(error);
    }
}

function showLoadFailure(error) {
    const message = `演示数据加载失败: ${error.message}`;
    toast(message);
    $("#summaryCards").innerHTML = `<div class="metric-card"><span>加载状态</span><strong>失败</strong></div>`;
    $("#auctionGrid").innerHTML = `<article class="auction-card"><span class="status-pill">检查环境</span><h3>无法读取演示库</h3><p>${message}</p><div class="auction-meta"><span>请先执行 ./scripts/deploy/init_demo_env.sh，再启动 ./scripts/deploy/run_demo_server.sh。</span></div></article>`;
    $("#roleTabs").innerHTML = "";
    $("#roleCard").innerHTML = `<p>${message}</p>`;
}

function renderDashboard(data) {
    setText("#generatedAt", `生成时间: ${safe(data.generatedAt)}`);
    renderSummary(data.summary || {});
    renderHero(data.auctions || []);
    renderAccounts(data.accounts || []);
    renderAuctions(data.auctions || []);
    renderList("#orderList", data.orders || [], order => ({
        title: `${order.order_no} · ${statusLabel(order.order_status)}`,
        meta: `${order.title}，买家 ${order.buyer_nickname}，卖家 ${order.seller_nickname}，成交 ${money(order.final_amount)} 元`,
    }));
    renderList("#paymentList", data.payments || [], payment => ({
        title: `${payment.payment_no} · ${statusLabel(payment.pay_status)}`,
        meta: `${payment.pay_channel}，金额 ${money(payment.pay_amount)} 元，流水 ${safe(payment.transaction_no)}`,
    }));
    renderList("#reviewList", data.reviews || [], review => ({
        title: `${review.reviewer_nickname} 评价 ${review.reviewee_nickname}`,
        meta: `${review.review_type}，${review.rating} 星，${review.content}`,
    }));
    renderRisks(data.riskScenarios || []);
    renderTable("#bidTable", data.bids || [], ["title", "bidder_nickname", "bid_amount", "bid_status", "bid_time"]);
    renderTable("#notificationTable", data.notifications || [], ["user_nickname", "notice_type", "title", "push_status", "created_at"]);
    renderTable("#taskTable", data.taskLogs || [], ["task_type", "biz_key", "task_status", "retry_count", "finished_at"]);
    renderTable("#operationTable", data.operationLogs || [], ["operator_role", "module_name", "operation_name", "result", "created_at"]);
    renderTable("#statisticsTable", data.statisticsDaily || [], ["stat_date", "auction_count", "sold_count", "unsold_count", "bid_count", "gmv_amount"]);
}

function renderSummary(summary) {
    $("#summaryCards").innerHTML = summaryMap.map(([key, label]) => {
        const value = key.includes("Amount") ? `${money(summary[key])}` : safe(summary[key], "0");
        return `<article class="metric-card"><span>${label}</span><strong>${value}</strong></article>`;
    }).join("");
}

function renderHero(auctions) {
    const running = auctions.find(auction => auction.auction_status === "RUNNING") || auctions[0];
    if (!running) {
        setText("#heroPrice", "--");
        setText("#heroBidder", "暂无拍卖数据");
        return;
    }
    setText("#heroPrice", `¥${money(running.current_price)}`);
    setText("#heroBidder", `${running.title} · ${safe(running.highest_bidder_nickname, "暂无最高出价者")}`);
}

function renderAccounts(accounts) {
    if (!accounts.length) {
        $("#roleTabs").innerHTML = "";
        $("#roleCard").innerHTML = "<p>暂无演示账号</p>";
        return;
    }
    $("#roleTabs").innerHTML = accounts.map((account, index) => {
        const active = index === state.activeAccountIndex ? "active" : "";
        return `<button class="${active}" type="button" data-account-index="${index}">${account.nickname}</button>`;
    }).join("");
    $("#roleTabs").querySelectorAll("button").forEach(button => {
        button.addEventListener("click", () => {
            state.activeAccountIndex = Number(button.dataset.accountIndex);
            renderAccounts(accounts);
        });
    });
    const account = accounts[state.activeAccountIndex] || accounts[0];
    $("#roleCard").innerHTML = `
        <p>${safe(account.demo_usage)}</p>
        <dl>
            <div><dt>用户名</dt><dd>${account.username}</dd></div>
            <div><dt>角色</dt><dd>${account.role_code}</dd></div>
            <div><dt>状态</dt><dd>${account.status}</dd></div>
            <div><dt>答辩用途</dt><dd>${account.nickname}</dd></div>
        </dl>
    `;
}

function renderAuctions(auctions) {
    $("#auctionGrid").innerHTML = auctions.map(auction => `
        <article class="auction-card">
            <span class="status-pill">${statusLabel(auction.auction_status)}</span>
            <h3>${auction.title}</h3>
            <p>${auction.description}</p>
            <div class="auction-price">¥${money(auction.current_price)}</div>
            <div class="auction-meta">
                <span>起拍价: <strong>¥${money(auction.start_price)}</strong></span>
                <span>最低加价: <strong>¥${money(auction.bid_step)}</strong></span>
                <span>最高出价者: <strong>${safe(auction.highest_bidder_nickname)}</strong></span>
                <span>出价记录: <strong>${safe(auction.bid_count, "0")} 条</strong></span>
                <span>延时保护: <strong>${safe(auction.anti_sniping_window_seconds, "0")} 秒窗口，顺延 ${safe(auction.extend_seconds, "0")} 秒</strong></span>
                <span>结束时间: <strong>${safe(auction.end_time)}</strong></span>
            </div>
        </article>
    `).join("");
}

function renderList(selector, items, mapper) {
    const container = $(selector);
    if (!items.length) {
        container.innerHTML = '<div class="list-item"><strong>暂无数据</strong><span>请检查演示数据是否已导入。</span></div>';
        return;
    }
    container.innerHTML = items.map(item => {
        const mapped = mapper(item);
        return `<div class="list-item"><strong>${mapped.title}</strong><span>${mapped.meta}</span></div>`;
    }).join("");
}

function renderRisks(items) {
    $("#riskGrid").innerHTML = items.map(item => `
        <article>
            <span class="evidence-tag">${item.evidence}</span>
            <h3>${item.title}</h3>
            <p>${item.description}</p>
        </article>
    `).join("");
}

function renderTable(selector, rows, columns) {
    const container = $(selector);
    if (!rows.length) {
        container.innerHTML = "<p>暂无数据</p>";
        return;
    }
    const head = columns.map(column => `<th>${column}</th>`).join("");
    const body = rows.map(row => `
        <tr>${columns.map(column => `<td>${safe(row[column], "")}</td>`).join("")}</tr>
    `).join("");
    container.innerHTML = `<table><thead><tr>${head}</tr></thead><tbody>${body}</tbody></table>`;
}

function playDemo() {
    const steps = Array.from(document.querySelectorAll("[data-demo-step]"));
    if (!steps.length) {
        return;
    }
    let index = 0;
    toast("开始按答辩顺序播放");
    const tick = () => {
        steps.forEach(step => step.classList.remove("is-playing"));
        const current = steps[index];
        current.classList.add("is-playing");
        current.scrollIntoView({ behavior: "smooth", block: "center" });
        index += 1;
        if (index < steps.length) {
            window.setTimeout(tick, 2200);
        } else {
            window.setTimeout(() => current.classList.remove("is-playing"), 2200);
        }
    };
    tick();
}

$("#refreshButton").addEventListener("click", loadDashboard);
$("#playDemoButton").addEventListener("click", playDemo);

loadDashboard();
