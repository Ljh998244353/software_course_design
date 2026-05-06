const API_BASE = "/api";
const TOKEN_KEY = "auction_app_token";

const routeRoles = {
    gate: [],
    home: ["USER", "ADMIN", "SUPPORT"],
    buyer: ["USER"],
    orders: ["USER"],
    notifications: ["USER", "ADMIN", "SUPPORT"],
    seller: ["USER"],
    admin: ["ADMIN"],
    support: ["SUPPORT"],
    api: ["USER", "ADMIN", "SUPPORT"],
};

const endpoints = [
    { name: "注册", path: "POST /api/auth/register" },
    { name: "登录", path: "POST /api/auth/login" },
    { name: "登出", path: "POST /api/auth/logout" },
    { name: "当前用户", path: "GET /api/auth/me" },
    { name: "修改用户状态", path: "PATCH /api/admin/users/{id}/status" },
    { name: "创建拍品", path: "POST /api/items" },
    { name: "编辑拍品", path: "PUT /api/items/{id}" },
    { name: "图片元数据", path: "POST /api/items/{id}/images" },
    { name: "我的拍品", path: "GET /api/items/mine" },
    { name: "提交审核", path: "POST /api/items/{id}/submit-audit" },
    { name: "待审核拍品", path: "GET /api/admin/items/pending" },
    { name: "审核拍品", path: "POST /api/admin/items/{id}/audit" },
    { name: "审核日志", path: "GET /api/admin/items/{id}/audit-logs" },
    { name: "创建拍卖", path: "POST /api/admin/auctions" },
    { name: "编辑拍卖", path: "PUT /api/admin/auctions/{id}" },
    { name: "取消拍卖", path: "POST /api/admin/auctions/{id}/cancel" },
    { name: "后台拍卖列表", path: "GET /api/admin/auctions" },
    { name: "后台拍卖详情", path: "GET /api/admin/auctions/{id}" },
    { name: "公开拍卖列表", path: "GET /api/auctions" },
    { name: "公开拍卖详情", path: "GET /api/auctions/{id}" },
    { name: "当前最高价", path: "GET /api/auctions/{id}/price" },
    { name: "出价历史", path: "GET /api/auctions/{id}/bids" },
    { name: "提交出价", path: "POST /api/auctions/{id}/bids" },
    { name: "我的出价状态", path: "GET /api/auctions/{id}/my-bid" },
    { name: "通知轮询", path: "GET /api/notifications" },
    { name: "通知已读", path: "PATCH /api/notifications/{id}/read" },
    { name: "我的订单", path: "GET /api/orders/mine" },
    { name: "订单详情", path: "GET /api/orders/{id}" },
    { name: "发起支付", path: "POST /api/orders/{id}/pay" },
    { name: "支付状态", path: "GET /api/orders/{id}/payment" },
    { name: "支付回调", path: "POST /api/payments/callback" },
    { name: "卖家发货", path: "POST /api/orders/{id}/ship" },
    { name: "确认收货", path: "POST /api/orders/{id}/confirm-receipt" },
    { name: "提交评价", path: "POST /api/reviews" },
    { name: "订单评价", path: "GET /api/orders/{id}/reviews" },
    { name: "信用摘要", path: "GET /api/users/{id}/reviews/summary" },
    { name: "上下文", path: "GET /api/system/context" },
];

const state = {
    token: window.sessionStorage.getItem(TOKEN_KEY) || "",
    user: null,
    route: "gate",
    buyerLoaded: false,
    publicAuctions: [],
    selectedPublicAuction: null,
    selectedPublicPrice: null,
    selectedBidHistory: [],
    selectedMyBidStatus: null,
    selectedNotifications: [],
    notificationsLoaded: false,
    notifications: [],
    ordersLoaded: false,
    orders: [],
    selectedOrder: null,
    selectedPaymentStatus: null,
    selectedOrderReviews: null,
    selectedOrderCredit: null,
    sellerLoaded: false,
    sellerItems: [],
    selectedItem: null,
    adminLoaded: false,
    pendingItems: [],
    adminAuctions: [],
    selectedAuction: null,
    auditLogs: [],
};

const statusClassMap = {
    REJECTED: "rejected",
    READY_FOR_AUCTION: "ready",
    PENDING_START: "ready",
    RUNNING: "running",
    SETTLING: "running",
    SOLD: "ready",
    UNSOLD: "rejected",
    CANCELLED: "rejected",
    PENDING_PAYMENT: "running",
    PAID: "ready",
    SHIPPED: "running",
    COMPLETED: "ready",
    REVIEWED: "ready",
    CLOSED: "rejected",
};

function $(selector) {
    return document.querySelector(selector);
}

function $all(selector) {
    return Array.from(document.querySelectorAll(selector));
}

function setStatus(label, kind = "") {
    const badge = $("#statusBadge");
    if (!badge) {
        return;
    }
    badge.textContent = label;
    badge.className = `status-badge${kind ? ` ${kind}` : ""}`;
}

function toast(message) {
    const element = $("#toast");
    element.textContent = message;
    element.classList.add("show");
    window.clearTimeout(toast.timer);
    toast.timer = window.setTimeout(() => element.classList.remove("show"), 2600);
}

function showResponse(payload) {
    const viewer = $("#responseViewer");
    if (viewer) {
        viewer.textContent = JSON.stringify(payload, null, 2);
    }
}

function toMoney(value) {
    const amount = Number(value || 0);
    return amount.toLocaleString("zh-CN", {
        minimumFractionDigits: 2,
        maximumFractionDigits: 2,
    });
}

function setText(selector, value) {
    const element = $(selector);
    if (element) {
        element.textContent = value;
    }
}

function clearSession() {
    state.token = "";
    state.user = null;
    state.buyerLoaded = false;
    state.publicAuctions = [];
    state.selectedPublicAuction = null;
    state.selectedPublicPrice = null;
    state.selectedBidHistory = [];
    state.selectedMyBidStatus = null;
    state.selectedNotifications = [];
    state.notificationsLoaded = false;
    state.notifications = [];
    state.ordersLoaded = false;
    state.orders = [];
    state.selectedOrder = null;
    state.selectedPaymentStatus = null;
    state.selectedOrderReviews = null;
    state.selectedOrderCredit = null;
    state.sellerLoaded = false;
    state.sellerItems = [];
    state.selectedItem = null;
    state.adminLoaded = false;
    state.pendingItems = [];
    state.adminAuctions = [];
    state.selectedAuction = null;
    state.auditLogs = [];
    window.sessionStorage.removeItem(TOKEN_KEY);
}

function storeSession(token, user) {
    state.token = token || "";
    state.user = user || null;
    if (state.token) {
        window.sessionStorage.setItem(TOKEN_KEY, state.token);
    }
}

function canEnter(route) {
    if (route === "gate") {
        return true;
    }
    if (!state.user) {
        return false;
    }
    return (routeRoles[route] || []).includes(state.user.roleCode);
}

function visibleRoutesForRole(roleCode) {
    return Object.entries(routeRoles)
        .filter(([route, roles]) => route !== "gate" && roles.includes(roleCode))
        .map(([route]) => route);
}

function syncHash(route) {
    const nextHash = route === "gate" ? "" : `#${route}`;
    if (window.location.hash !== nextHash) {
        window.history.replaceState(null, "", `${window.location.pathname}${nextHash}`);
    }
}

function setRoute(route, options = {}) {
    const requestedRoute = routeRoles[route] ? route : "home";
    if (requestedRoute === "gate" && state.user) {
        state.route = "home";
        render();
        syncHash("home");
        return;
    }
    if (!canEnter(requestedRoute)) {
        state.route = "gate";
        render();
        syncHash("gate");
        if (!options.silent) {
            toast("请先登录");
        }
        return;
    }

    state.route = requestedRoute;
    render();
    syncHash(requestedRoute);
    if (requestedRoute === "seller" && !state.sellerLoaded) {
        loadMyItems({ silent: true });
    }
    if (requestedRoute === "buyer" && !state.buyerLoaded) {
        loadPublicAuctions({ silent: true });
    }
    if (requestedRoute === "orders" && !state.ordersLoaded) {
        loadMyOrders({ silent: true });
    }
    if (requestedRoute === "notifications" && !state.notificationsLoaded) {
        loadNotifications({ silent: true });
    }
    if (requestedRoute === "admin" && !state.adminLoaded) {
        loadAdminDashboard({ silent: true });
    }
}

function renderNavigation() {
    const role = state.user?.roleCode || "";
    $all("[data-roles]").forEach(button => {
        const allowed = button.dataset.roles.split(",");
        button.hidden = !state.user || !allowed.includes(role);
        button.classList.toggle("active", button.dataset.route === state.route);
    });

    $all("[data-role-entry]").forEach(panel => {
        panel.hidden = panel.dataset.roleEntry !== role;
    });
}

function renderSession() {
    const label = $("#sessionLabel");
    const logoutButton = $("#logoutButton");

    if (!state.user) {
        label.textContent = "未登录";
        logoutButton.hidden = true;
        return;
    }

    label.textContent = `${state.user.username} / ${state.user.roleCode}`;
    logoutButton.hidden = false;
}

function renderProfile() {
    const user = state.user;
    $("#profileName").textContent = user ? (user.nickname || user.username) : "当前用户";
    $("#profileUserId").textContent = user ? String(user.userId) : "-";
    $("#profileRole").textContent = user ? user.roleCode : "-";
    $("#profileStatus").textContent = user ? user.status : "-";
    $("#profileExpireAt").textContent = user ? (user.tokenExpireAt || "-") : "-";
}

function renderEndpoints() {
    const list = $("#endpointList");
    if (!list) {
        return;
    }
    list.innerHTML = "";
    endpoints.forEach(item => {
        const row = document.createElement("div");
        const name = document.createElement("dt");
        const path = document.createElement("dd");
        name.textContent = item.name;
        path.textContent = item.path;
        row.append(name, path);
        list.append(row);
    });
}

function resetItemForm() {
    $("#itemEditIdInput").value = "";
    $("#itemFormTitle").textContent = "新建拍品";
    $("#itemTitleInput").value = "";
    $("#itemDescriptionInput").value = "";
    $("#itemCategoryInput").value = "1";
    $("#itemStartPriceInput").value = "";
    $("#itemCoverInput").value = "";
}

function fillItemForm(item) {
    $("#itemEditIdInput").value = String(item.itemId);
    $("#itemFormTitle").textContent = `编辑 #${item.itemId}`;
    $("#itemTitleInput").value = item.title || "";
    $("#itemDescriptionInput").value = item.description || "";
    $("#itemCategoryInput").value = String(item.categoryId || 1);
    $("#itemStartPriceInput").value = item.startPrice ? String(item.startPrice) : "";
    $("#itemCoverInput").value = item.coverImageUrl || "";
}

function renderSellerItems() {
    const list = $("#sellerItemList");
    if (!list) {
        return;
    }
    list.innerHTML = "";

    if (!state.sellerItems.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无拍品";
        list.append(empty);
        return;
    }

    state.sellerItems.forEach(item => {
        const card = document.createElement("article");
        card.className = "item-card";
        if (state.selectedItem?.item?.itemId === item.itemId) {
            card.classList.add("selected");
        }

        const head = document.createElement("div");
        head.className = "item-card-head";

        const titleBox = document.createElement("div");
        const title = document.createElement("h3");
        title.textContent = item.title;
        const meta = document.createElement("div");
        meta.className = "item-meta";
        const price = document.createElement("span");
        price.textContent = `起拍 ${toMoney(item.startPrice)}`;
        const updated = document.createElement("span");
        updated.textContent = item.updatedAt || item.createdAt || "-";
        meta.append(price, updated);
        titleBox.append(title, meta);

        const status = document.createElement("span");
        status.className = `item-status ${statusClassMap[item.itemStatus] || ""}`.trim();
        status.textContent = item.itemStatus;
        head.append(titleBox, status);

        if (item.rejectReason) {
            const reason = document.createElement("div");
            reason.className = "empty-state";
            reason.textContent = item.rejectReason;
            card.append(head, reason);
        } else {
            card.append(head);
        }

        const actions = document.createElement("div");
        actions.className = "item-card-actions";
        const detailButton = document.createElement("button");
        detailButton.className = "text-button";
        detailButton.type = "button";
        detailButton.textContent = "详情";
        detailButton.addEventListener("click", () => selectItem(item.itemId));

        const editButton = document.createElement("button");
        editButton.className = "text-button";
        editButton.type = "button";
        editButton.textContent = "编辑";
        editButton.disabled = !["DRAFT", "REJECTED"].includes(item.itemStatus);
        editButton.addEventListener("click", async () => {
            await selectItem(item.itemId);
            if (state.selectedItem?.item) {
                fillItemForm(state.selectedItem.item);
            }
        });

        const submitButton = document.createElement("button");
        submitButton.className = "secondary-button";
        submitButton.type = "button";
        submitButton.textContent = "提交审核";
        submitButton.disabled = item.itemStatus !== "DRAFT";
        submitButton.addEventListener("click", async () => {
            await selectItem(item.itemId);
            await submitSelectedItem();
        });

        actions.append(detailButton, editButton, submitButton);
        card.append(actions);
        list.append(card);
    });
}

function renderSelectedItem() {
    const detail = state.selectedItem;
    const detailPanel = $("#itemDetailPanel");
    const detailList = $("#itemDetailList");
    const imageList = $("#itemImageList");
    if (!detailPanel || !detailList || !imageList) {
        return;
    }

    if (!detail?.item) {
        detailPanel.hidden = true;
        setText("#selectedItemLabel", "未选择拍品");
        return;
    }

    const item = detail.item;
    detailPanel.hidden = false;
    setText("#selectedItemLabel", `#${item.itemId} / ${item.itemStatus}`);
    $("#imageItemIdInput").value = String(item.itemId);

    detailList.innerHTML = "";
    [
        ["标题", item.title],
        ["状态", item.itemStatus],
        ["起拍价", toMoney(item.startPrice)],
        ["驳回原因", item.rejectReason || "-"],
        ["最近审核", detail.latestAuditLog?.auditResult || "-"],
    ].forEach(([label, value]) => {
        const row = document.createElement("div");
        const dt = document.createElement("dt");
        const dd = document.createElement("dd");
        dt.textContent = label;
        dd.textContent = value;
        row.append(dt, dd);
        detailList.append(row);
    });

    imageList.innerHTML = "";
    if (!detail.images?.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无图片";
        imageList.append(empty);
        return;
    }

    detail.images.forEach(image => {
        const row = document.createElement("div");
        row.className = "image-row";
        const label = document.createElement("span");
        label.textContent = `${image.isCover ? "封面" : "图片"} #${image.imageId} ${image.imageUrl}`;
        const button = document.createElement("button");
        button.className = "text-button";
        button.type = "button";
        button.textContent = "删除";
        button.disabled = !["DRAFT", "REJECTED"].includes(item.itemStatus);
        button.addEventListener("click", () => deleteItemImage(image.imageId));
        row.append(label, button);
        imageList.append(row);
    });
}

function renderSeller() {
    renderSellerItems();
    renderSelectedItem();
}

function toApiDateTime(value) {
    const normalized = String(value || "").trim();
    if (!normalized) {
        return "";
    }
    const text = normalized.replace("T", " ");
    return text.length === 16 ? `${text}:00` : text;
}

function toDateTimeLocalValue(value) {
    const normalized = String(value || "").trim();
    if (!normalized) {
        return "";
    }
    return normalized.replace(" ", "T").slice(0, 16);
}

function renderPendingItems() {
    const list = $("#adminPendingItemList");
    if (!list) {
        return;
    }
    list.innerHTML = "";

    if (!state.pendingItems.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无待审核拍品";
        list.append(empty);
        return;
    }

    state.pendingItems.forEach(item => {
        const card = document.createElement("article");
        card.className = "item-card";

        const head = document.createElement("div");
        head.className = "item-card-head";

        const titleBox = document.createElement("div");
        const title = document.createElement("h3");
        title.textContent = item.title;
        const meta = document.createElement("div");
        meta.className = "item-meta";
        const seller = document.createElement("span");
        seller.textContent = `卖家 ${item.sellerUsername || item.sellerId}`;
        const price = document.createElement("span");
        price.textContent = `建议起拍 ${toMoney(item.startPrice)}`;
        const created = document.createElement("span");
        created.textContent = item.createdAt || "-";
        meta.append(seller, price, created);
        titleBox.append(title, meta);

        const status = document.createElement("span");
        status.className = "item-status";
        status.textContent = item.itemStatus || "PENDING_AUDIT";
        head.append(titleBox, status);

        const actions = document.createElement("div");
        actions.className = "item-card-actions";
        const auditButton = document.createElement("button");
        auditButton.className = "secondary-button";
        auditButton.type = "button";
        auditButton.textContent = "审核";
        auditButton.addEventListener("click", () => prepareAudit(item.itemId));

        const logButton = document.createElement("button");
        logButton.className = "text-button";
        logButton.type = "button";
        logButton.textContent = "日志";
        logButton.addEventListener("click", () => loadAuditLogs(item.itemId));

        actions.append(auditButton, logButton);
        card.append(head, actions);
        list.append(card);
    });
}

function renderAuditLogs() {
    const panel = $("#adminAuditLogsPanel");
    if (!panel) {
        return;
    }
    panel.innerHTML = "";

    if (!state.auditLogs.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无审核日志";
        panel.append(empty);
        return;
    }

    state.auditLogs.forEach(log => {
        const row = document.createElement("div");
        row.className = "image-row";
        const label = document.createElement("span");
        label.textContent = `#${log.auditLogId} ${log.auditResult} / ${log.adminUsername || log.adminId} / ${log.createdAt}`;
        const reason = document.createElement("span");
        reason.textContent = log.auditComment || "-";
        row.append(label, reason);
        panel.append(row);
    });
}

function prepareAudit(itemId) {
    $("#auditItemIdInput").value = String(itemId);
    $("#auditStatusSelect").value = "APPROVED";
    $("#auditReasonInput").value = "";
    loadAuditLogs(itemId);
}

function resetAuctionForm() {
    $("#auctionEditIdInput").value = "";
    $("#auctionFormTitle").textContent = "创建拍卖";
    $("#auctionItemIdInput").value = "";
    $("#auctionItemIdInput").disabled = false;
    $("#auctionStartTimeInput").value = "";
    $("#auctionEndTimeInput").value = "";
    $("#auctionStartPriceInput").value = "";
    $("#auctionBidStepInput").value = "";
    $("#auctionWindowInput").value = "0";
    $("#auctionExtendInput").value = "0";
}

function fillAuctionForm(detail) {
    const auction = detail?.auction || detail;
    if (!auction) {
        return;
    }
    $("#auctionEditIdInput").value = String(auction.auctionId);
    $("#auctionFormTitle").textContent = `编辑 #${auction.auctionId}`;
    $("#auctionItemIdInput").value = String(auction.itemId);
    $("#auctionItemIdInput").disabled = true;
    $("#auctionStartTimeInput").value = toDateTimeLocalValue(auction.startTime);
    $("#auctionEndTimeInput").value = toDateTimeLocalValue(auction.endTime);
    $("#auctionStartPriceInput").value = String(auction.startPrice || "");
    $("#auctionBidStepInput").value = String(auction.bidStep || "");
    $("#auctionWindowInput").value = String(auction.antiSnipingWindowSeconds || 0);
    $("#auctionExtendInput").value = String(auction.extendSeconds || 0);
}

function readAuctionFormPayload() {
    return {
        itemId: Number($("#auctionItemIdInput").value),
        startTime: toApiDateTime($("#auctionStartTimeInput").value),
        endTime: toApiDateTime($("#auctionEndTimeInput").value),
        startPrice: Number($("#auctionStartPriceInput").value),
        bidStep: Number($("#auctionBidStepInput").value),
        antiSnipingWindowSeconds: Number($("#auctionWindowInput").value || 0),
        extendSeconds: Number($("#auctionExtendInput").value || 0),
    };
}

function renderAdminAuctions() {
    const list = $("#adminAuctionList");
    if (!list) {
        return;
    }
    list.innerHTML = "";

    if (!state.adminAuctions.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无拍卖";
        list.append(empty);
        return;
    }

    state.adminAuctions.forEach(auction => {
        const card = document.createElement("article");
        card.className = "item-card";
        if (state.selectedAuction?.auction?.auctionId === auction.auctionId) {
            card.classList.add("selected");
        }

        const head = document.createElement("div");
        head.className = "item-card-head";

        const titleBox = document.createElement("div");
        const title = document.createElement("h3");
        title.textContent = auction.title || `拍卖 #${auction.auctionId}`;
        const meta = document.createElement("div");
        meta.className = "item-meta";
        const itemId = document.createElement("span");
        itemId.textContent = `拍品 #${auction.itemId}`;
        const price = document.createElement("span");
        price.textContent = `当前 ${toMoney(auction.currentPrice)}`;
        const time = document.createElement("span");
        time.textContent = `${auction.startTime} - ${auction.endTime}`;
        meta.append(itemId, price, time);
        titleBox.append(title, meta);

        const status = document.createElement("span");
        status.className = `item-status ${statusClassMap[auction.status] || ""}`.trim();
        status.textContent = auction.status;
        head.append(titleBox, status);

        const actions = document.createElement("div");
        actions.className = "item-card-actions";
        const detailButton = document.createElement("button");
        detailButton.className = "text-button";
        detailButton.type = "button";
        detailButton.textContent = "详情";
        detailButton.addEventListener("click", () => selectAuction(auction.auctionId));

        const editButton = document.createElement("button");
        editButton.className = "text-button";
        editButton.type = "button";
        editButton.textContent = "编辑";
        editButton.disabled = auction.status !== "PENDING_START";
        editButton.addEventListener("click", async () => {
            await selectAuction(auction.auctionId);
            fillAuctionForm(state.selectedAuction);
        });

        const cancelButton = document.createElement("button");
        cancelButton.className = "secondary-button";
        cancelButton.type = "button";
        cancelButton.textContent = "取消";
        cancelButton.disabled = auction.status !== "PENDING_START";
        cancelButton.addEventListener("click", () => cancelAuction(auction.auctionId));

        actions.append(detailButton, editButton, cancelButton);
        card.append(head, actions);
        list.append(card);
    });
}

function appendDetailRow(list, label, value) {
    const row = document.createElement("div");
    const dt = document.createElement("dt");
    const dd = document.createElement("dd");
    dt.textContent = label;
    dd.textContent = value;
    row.append(dt, dd);
    list.append(row);
}

function renderSelectedAuction() {
    const panel = $("#adminAuctionDetailPanel");
    if (!panel) {
        return;
    }

    panel.innerHTML = "";
    const detail = state.selectedAuction;
    if (!detail?.auction) {
        panel.hidden = true;
        return;
    }
    panel.hidden = false;

    const list = document.createElement("dl");
    list.className = "profile-list";
    appendDetailRow(list, "拍卖 ID", String(detail.auction.auctionId));
    appendDetailRow(list, "状态", detail.auction.status);
    appendDetailRow(list, "拍品", `${detail.item?.title || "-"} / #${detail.auction.itemId}`);
    appendDetailRow(list, "卖家", detail.sellerUsername || String(detail.auction.sellerId));
    appendDetailRow(list, "当前价", toMoney(detail.auction.currentPrice));
    appendDetailRow(list, "加价幅度", toMoney(detail.auction.bidStep));
    appendDetailRow(list, "时间", `${detail.auction.startTime} - ${detail.auction.endTime}`);
    appendDetailRow(
        list,
        "防狙击",
        `${detail.auction.antiSnipingWindowSeconds}s / ${detail.auction.extendSeconds}s`
    );

    const actions = document.createElement("div");
    actions.className = "item-card-actions auction-detail-actions";
    const editButton = document.createElement("button");
    editButton.className = "text-button";
    editButton.type = "button";
    editButton.textContent = "编辑";
    editButton.disabled = detail.auction.status !== "PENDING_START";
    editButton.addEventListener("click", () => fillAuctionForm(detail));

    const cancelButton = document.createElement("button");
    cancelButton.className = "secondary-button";
    cancelButton.type = "button";
    cancelButton.textContent = "取消";
    cancelButton.disabled = detail.auction.status !== "PENDING_START";
    cancelButton.addEventListener("click", () => cancelAuction(detail.auction.auctionId));

    actions.append(editButton, cancelButton);
    panel.append(list, actions);
}

function renderAdmin() {
    renderPendingItems();
    renderAuditLogs();
    renderAdminAuctions();
    renderSelectedAuction();
}

function parseDateTime(value) {
    const normalized = String(value || "").replace(" ", "T");
    const date = new Date(normalized);
    return Number.isNaN(date.getTime()) ? null : date;
}

function formatDuration(milliseconds) {
    const totalSeconds = Math.max(0, Math.floor(milliseconds / 1000));
    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;
    if (hours > 0) {
        return `${hours}小时${String(minutes).padStart(2, "0")}分`;
    }
    return `${minutes}分${String(seconds).padStart(2, "0")}秒`;
}

function auctionCountdownText(auction) {
    const now = Date.now();
    if (auction.status === "PENDING_START") {
        const start = parseDateTime(auction.startTime);
        return start ? `距开始 ${formatDuration(start.getTime() - now)}` : "待开始";
    }
    if (auction.status === "RUNNING") {
        const end = parseDateTime(auction.endTime);
        return end ? `距结束 ${formatDuration(end.getTime() - now)}` : "竞拍中";
    }
    if (["SOLD", "UNSOLD", "CANCELLED"].includes(auction.status)) {
        return "已结束";
    }
    return auction.status || "-";
}

function auctionActionLabel(status) {
    if (status === "RUNNING") {
        return "出价";
    }
    if (status === "PENDING_START") {
        return "未开始";
    }
    if (status === "SETTLING") {
        return "结算中";
    }
    return "不可出价";
}

function renderPublicAuctions() {
    const list = $("#buyerAuctionList");
    if (!list) {
        return;
    }
    list.innerHTML = "";

    if (!state.publicAuctions.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无拍卖";
        list.append(empty);
        return;
    }

    state.publicAuctions.forEach(auction => {
        const card = document.createElement("article");
        card.className = "item-card";
        if (state.selectedPublicAuction?.auctionId === auction.auctionId) {
            card.classList.add("selected");
        }

        const head = document.createElement("div");
        head.className = "item-card-head";

        const titleBox = document.createElement("div");
        const title = document.createElement("h3");
        title.textContent = auction.title || `拍卖 #${auction.auctionId}`;
        const meta = document.createElement("div");
        meta.className = "item-meta";
        const price = document.createElement("span");
        price.textContent = `当前 ${toMoney(auction.currentPrice)}`;
        const countdown = document.createElement("span");
        countdown.textContent = auctionCountdownText(auction);
        const time = document.createElement("span");
        time.textContent = `${auction.startTime} - ${auction.endTime}`;
        meta.append(price, countdown, time);
        titleBox.append(title, meta);

        const status = document.createElement("span");
        status.className = `item-status ${statusClassMap[auction.status] || ""}`.trim();
        status.textContent = auction.status;
        head.append(titleBox, status);

        const actions = document.createElement("div");
        actions.className = "item-card-actions";
        const detailButton = document.createElement("button");
        detailButton.className = "secondary-button";
        detailButton.type = "button";
        detailButton.textContent = "详情";
        detailButton.addEventListener("click", () => selectPublicAuction(auction.auctionId));

        const bidButton = document.createElement("button");
        bidButton.className = "text-button";
        bidButton.type = "button";
        bidButton.textContent = auctionActionLabel(auction.status);
        bidButton.disabled = auction.status !== "RUNNING";
        bidButton.addEventListener("click", () => selectPublicAuction(auction.auctionId));

        actions.append(detailButton, bidButton);
        card.append(head, actions);
        list.append(card);
    });
}

function isAuctionAcceptingBids(detail) {
    return detail?.status === "RUNNING";
}

function buildOutbidNotice() {
    const myStatus = state.selectedMyBidStatus;
    if (!myStatus || !myStatus.myHighestBid || myStatus.isCurrentHighest) {
        return "";
    }
    return `已被超越，当前最高价 ${toMoney(myStatus.currentPrice)}`;
}

function renderPublicAuctionDetail() {
    const panel = $("#buyerAuctionDetailPanel");
    if (!panel) {
        return;
    }

    panel.innerHTML = "";
    const detail = state.selectedPublicAuction;
    if (!detail) {
        panel.hidden = true;
        return;
    }
    panel.hidden = false;

    const list = document.createElement("dl");
    list.className = "profile-list";
    appendDetailRow(list, "拍卖 ID", String(detail.auctionId));
    appendDetailRow(list, "拍品", `${detail.title || "-"} / #${detail.itemId}`);
    appendDetailRow(list, "状态", detail.status);
    appendDetailRow(list, "当前价", toMoney(state.selectedPublicPrice?.currentPrice || detail.currentPrice));
    appendDetailRow(
        list,
        "下一口",
        toMoney(state.selectedPublicPrice?.minimumNextBid || detail.currentPrice)
    );
    appendDetailRow(
        list,
        "最高出价者",
        state.selectedPublicPrice?.highestBidderMasked || detail.highestBidderMasked || "-"
    );
    appendDetailRow(list, "倒计时", auctionCountdownText(detail));
    appendDetailRow(list, "时间", `${detail.startTime} - ${state.selectedPublicPrice?.endTime || detail.endTime}`);
    appendDetailRow(
        list,
        "防狙击",
        `${detail.antiSnipingWindowSeconds}s / ${detail.extendSeconds}s`
    );

    const bidPanel = document.createElement("section");
    bidPanel.className = "bid-action-panel";

    const bidForm = document.createElement("form");
    bidForm.className = "bid-form";
    const label = document.createElement("label");
    const labelText = document.createElement("span");
    labelText.textContent = "出价金额";
    const input = document.createElement("input");
    input.id = "buyerBidAmountInput";
    input.name = "bidAmount";
    input.type = "number";
    input.min = "0.01";
    input.step = "0.01";
    input.value = String(state.selectedPublicPrice?.minimumNextBid || detail.currentPrice || "");
    label.append(labelText, input);

    const bidButton = document.createElement("button");
    bidButton.className = "primary-button";
    bidButton.type = "submit";
    bidButton.disabled = !isAuctionAcceptingBids(detail);
    bidButton.textContent = auctionActionLabel(detail.status);
    bidForm.append(label, bidButton);
    bidForm.addEventListener("submit", submitPublicBid);
    bidPanel.append(bidForm);

    const outbidNotice = buildOutbidNotice();
    if (
        outbidNotice ||
        (state.selectedMyBidStatus?.myHighestBid > 0 && state.selectedMyBidStatus.isCurrentHighest)
    ) {
        const status = document.createElement("div");
        status.className = `bid-alert${outbidNotice ? " warning" : " success"}`;
        status.textContent = outbidNotice ||
            `当前领先，最高出价 ${toMoney(state.selectedMyBidStatus.myHighestBid)}`;
        bidPanel.append(status);
    }

    if (state.selectedNotifications.length) {
        const notificationList = document.createElement("div");
        notificationList.className = "notification-list";
        state.selectedNotifications.slice(0, 3).forEach(notification => {
            const row = document.createElement("div");
            row.className = "notification-row";
            const title = document.createElement("strong");
            title.textContent = notification.title || notification.noticeType;
            const content = document.createElement("span");
            content.textContent = `${notification.content || ""} / ${notification.createdAt || "-"}`;
            row.append(title, content);
            notificationList.append(row);
        });
        bidPanel.append(notificationList);
    }

    const historySection = document.createElement("section");
    historySection.className = "bid-history-panel";
    const historyTitle = document.createElement("h2");
    historyTitle.textContent = "出价历史";
    historySection.append(historyTitle);

    if (!state.selectedBidHistory.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无出价记录";
        historySection.append(empty);
    } else {
        const rows = document.createElement("div");
        rows.className = "image-list";
        state.selectedBidHistory.forEach(entry => {
            const row = document.createElement("div");
            row.className = "image-row";
            const label = document.createElement("span");
            label.textContent = `${entry.bidderMasked || "-"} / ${entry.bidStatus}`;
            const value = document.createElement("span");
            value.textContent = `${toMoney(entry.bidAmount)} / ${entry.bidTime}`;
            row.append(label, value);
            rows.append(row);
        });
        historySection.append(rows);
    }

    panel.append(list, bidPanel, historySection);
}

function renderBuyer() {
    renderPublicAuctions();
    renderPublicAuctionDetail();
}

function getOrderRecord(entry) {
    return entry?.order || entry || {};
}

function currentUserId() {
    return Number(state.user?.userId || 0);
}

function isCurrentBuyer(entry) {
    const order = getOrderRecord(entry);
    return Number(order.buyerId || entry?.buyerId || 0) === currentUserId();
}

function isCurrentSeller(entry) {
    const order = getOrderRecord(entry);
    return Number(order.sellerId || entry?.sellerId || 0) === currentUserId();
}

function orderStatus(entry) {
    const order = getOrderRecord(entry);
    return order.orderStatus || entry?.orderStatus || "";
}

function latestPaymentForSelectedOrder() {
    return state.selectedPaymentStatus?.latestPayment || state.selectedOrder?.latestPayment || null;
}

function selectedOrderId() {
    return state.selectedOrder?.orderId || state.selectedOrder?.order?.orderId || 0;
}

function reviewTargetId(entry) {
    const order = getOrderRecord(entry);
    return isCurrentBuyer(entry)
        ? Number(order.sellerId || entry?.sellerId || 0)
        : Number(order.buyerId || entry?.buyerId || 0);
}

function reviewTargetLabel(entry) {
    if (isCurrentBuyer(entry)) {
        return entry?.sellerUsername || `卖家 #${reviewTargetId(entry)}`;
    }
    return entry?.buyerUsername || `买家 #${reviewTargetId(entry)}`;
}

function currentUserReviewForSelectedOrder() {
    const reviews = state.selectedOrderReviews?.reviews || [];
    return reviews.find(review => Number(review.reviewerId || 0) === currentUserId()) || null;
}

function renderReviewStars(rating) {
    const value = Math.max(0, Math.min(5, Number(rating || 0)));
    return "★★★★★".slice(0, value) + "☆☆☆☆☆".slice(0, 5 - value);
}

function renderOrders() {
    const list = $("#orderList");
    if (!list) {
        return;
    }
    list.innerHTML = "";

    if (!state.orders.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无订单";
        list.append(empty);
        return;
    }

    state.orders.forEach(entry => {
        const order = getOrderRecord(entry);
        const card = document.createElement("article");
        card.className = "item-card";
        if (state.selectedOrder?.orderId === entry.orderId) {
            card.classList.add("selected");
        }

        const head = document.createElement("div");
        head.className = "item-card-head";
        const titleBox = document.createElement("div");
        const title = document.createElement("h3");
        title.textContent = entry.itemTitle || `订单 #${entry.orderId}`;
        const meta = document.createElement("div");
        meta.className = "item-meta";
        const amount = document.createElement("span");
        amount.textContent = `成交 ${toMoney(order.finalAmount || entry.finalAmount)}`;
        const role = document.createElement("span");
        role.textContent = isCurrentBuyer(entry) ? "买家" : "卖家";
        const updated = document.createElement("span");
        updated.textContent = order.updatedAt || "-";
        meta.append(amount, role, updated);
        titleBox.append(title, meta);

        const status = document.createElement("span");
        status.className = `item-status ${statusClassMap[orderStatus(entry)] || ""}`.trim();
        status.textContent = orderStatus(entry);
        head.append(titleBox, status);

        const actions = document.createElement("div");
        actions.className = "item-card-actions";
        const detailButton = document.createElement("button");
        detailButton.className = "secondary-button";
        detailButton.type = "button";
        detailButton.textContent = "详情";
        detailButton.addEventListener("click", () => selectOrder(entry.orderId));
        actions.append(detailButton);

        card.append(head, actions);
        list.append(card);
    });
}

function renderSelectedOrder() {
    const panel = $("#orderDetailPanel");
    if (!panel) {
        return;
    }
    panel.innerHTML = "";

    const entry = state.selectedOrder;
    if (!entry) {
        panel.hidden = true;
        return;
    }
    panel.hidden = false;

    const order = getOrderRecord(entry);
    const list = document.createElement("dl");
    list.className = "profile-list";
    appendDetailRow(list, "订单 ID", String(entry.orderId || order.orderId));
    appendDetailRow(list, "订单号", order.orderNo || entry.orderNo || "-");
    appendDetailRow(list, "拍品", `${entry.itemTitle || "-"} / #${entry.itemId || "-"}`);
    appendDetailRow(list, "订单状态", orderStatus(entry));
    appendDetailRow(list, "拍卖状态", entry.auctionStatus || "-");
    appendDetailRow(list, "成交金额", toMoney(order.finalAmount || entry.finalAmount));
    appendDetailRow(list, "买家", entry.buyerUsername || String(order.buyerId || "-"));
    appendDetailRow(list, "卖家", entry.sellerUsername || String(order.sellerId || "-"));
    appendDetailRow(list, "支付截止", order.payDeadlineAt || entry.payDeadlineAt || "-");
    appendDetailRow(list, "支付时间", order.paidAt || "-");
    appendDetailRow(list, "发货时间", order.shippedAt || "-");
    appendDetailRow(list, "完成时间", order.completedAt || "-");
    panel.append(list);

    const paymentPanel = document.createElement("section");
    paymentPanel.className = "payment-status-panel";
    const payment = latestPaymentForSelectedOrder();
    const paymentTitle = document.createElement("strong");
    paymentTitle.textContent = "最新支付";
    paymentPanel.append(paymentTitle);
    if (payment) {
        const paymentText = document.createElement("code");
        paymentText.textContent = `${payment.paymentNo} / ${payment.payChannel} / ${payment.payStatus} / ${toMoney(payment.payAmount)}`;
        paymentPanel.append(paymentText);
    } else {
        const empty = document.createElement("span");
        empty.textContent = "暂无支付单";
        paymentPanel.append(empty);
    }
    panel.append(paymentPanel);

    const actions = document.createElement("div");
    actions.className = "item-card-actions auction-detail-actions";

    if (isCurrentBuyer(entry) && orderStatus(entry) === "PENDING_PAYMENT") {
        const channel = document.createElement("select");
        channel.id = "orderPayChannelSelect";
        ["MOCK_ALIPAY", "MOCK_WECHAT"].forEach(value => {
            const option = document.createElement("option");
            option.value = value;
            option.textContent = value;
            channel.append(option);
        });
        const payButton = document.createElement("button");
        payButton.id = "payOrderButton";
        payButton.className = "primary-button";
        payButton.type = "button";
        payButton.textContent = "发起支付";
        payButton.addEventListener("click", () => initiateOrderPayment(channel.value));
        actions.append(channel, payButton);
    }

    const refreshPaymentButton = document.createElement("button");
    refreshPaymentButton.className = "text-button";
    refreshPaymentButton.type = "button";
    refreshPaymentButton.textContent = "刷新支付状态";
    refreshPaymentButton.addEventListener("click", () => refreshSelectedOrder());
    actions.append(refreshPaymentButton);

    const shipButton = document.createElement("button");
    shipButton.id = "shipOrderButton";
    shipButton.className = "secondary-button";
    shipButton.type = "button";
    shipButton.textContent = "确认发货";
    shipButton.disabled = !isCurrentSeller(entry) || orderStatus(entry) !== "PAID";
    shipButton.addEventListener("click", shipSelectedOrder);

    const receiptButton = document.createElement("button");
    receiptButton.id = "confirmReceiptButton";
    receiptButton.className = "secondary-button";
    receiptButton.type = "button";
    receiptButton.textContent = "确认收货";
    receiptButton.disabled = !isCurrentBuyer(entry) || orderStatus(entry) !== "SHIPPED";
    receiptButton.addEventListener("click", confirmSelectedReceipt);

    actions.append(shipButton, receiptButton);
    panel.append(actions);

    renderSelectedOrderReviews(panel, entry);
}

function renderOrderPage() {
    renderOrders();
    renderSelectedOrder();
}

function renderSelectedOrderReviews(panel, entry) {
    const reviewSection = document.createElement("section");
    reviewSection.className = "review-panel";

    const title = document.createElement("h2");
    title.textContent = "评价与信用";
    reviewSection.append(title);

    const credit = state.selectedOrderCredit;
    if (credit) {
        const creditBox = document.createElement("div");
        creditBox.className = "credit-summary";
        const target = document.createElement("strong");
        target.textContent = reviewTargetLabel(entry);
        const score = document.createElement("span");
        score.textContent = `${Number(credit.averageRating || 0).toFixed(1)} 分 / ${credit.totalReviews || 0} 条`;
        const counts = document.createElement("span");
        counts.textContent =
            `好评 ${credit.positiveReviews || 0} / 中评 ${credit.neutralReviews || 0} / 差评 ${credit.negativeReviews || 0}`;
        creditBox.append(target, score, counts);
        reviewSection.append(creditBox);
    }

    const reviews = state.selectedOrderReviews?.reviews || [];
    if (!reviews.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无评价";
        reviewSection.append(empty);
    } else {
        const list = document.createElement("div");
        list.className = "review-list";
        reviews.forEach(review => {
            const row = document.createElement("div");
            row.className = "review-row";
            const head = document.createElement("strong");
            head.textContent = `${review.reviewType} / ${renderReviewStars(review.rating)}`;
            const content = document.createElement("span");
            content.textContent = `${review.content || "无评价内容"} / ${review.createdAt || "-"}`;
            row.append(head, content);
            list.append(row);
        });
        reviewSection.append(list);
    }

    const status = orderStatus(entry);
    const ownReview = currentUserReviewForSelectedOrder();
    if (["COMPLETED", "REVIEWED"].includes(status) && !ownReview) {
        const form = document.createElement("form");
        form.className = "review-form";
        const ratingLabel = document.createElement("label");
        const ratingText = document.createElement("span");
        ratingText.textContent = "评分";
        const ratingSelect = document.createElement("select");
        ratingSelect.id = "reviewRatingSelect";
        [5, 4, 3, 2, 1].forEach(value => {
            const option = document.createElement("option");
            option.value = String(value);
            option.textContent = `${value} 分`;
            ratingSelect.append(option);
        });
        ratingLabel.append(ratingText, ratingSelect);

        const contentLabel = document.createElement("label");
        const contentText = document.createElement("span");
        contentText.textContent = "评价内容";
        const textarea = document.createElement("textarea");
        textarea.id = "reviewContentInput";
        textarea.name = "content";
        textarea.rows = 3;
        textarea.maxLength = 500;
        contentLabel.append(contentText, textarea);

        const submit = document.createElement("button");
        submit.className = "primary-button";
        submit.type = "submit";
        submit.textContent = `评价 ${reviewTargetLabel(entry)}`;
        form.append(ratingLabel, contentLabel, submit);
        form.addEventListener("submit", submitSelectedOrderReview);
        reviewSection.append(form);
    } else if (ownReview) {
        const submitted = document.createElement("div");
        submitted.className = "bid-alert success";
        submitted.textContent = "当前账号已完成本订单评价";
        reviewSection.append(submitted);
    }

    panel.append(reviewSection);
}

function renderNotifications() {
    const list = $("#notificationCenterList");
    if (!list) {
        return;
    }
    list.innerHTML = "";

    if (!state.notifications.length) {
        const empty = document.createElement("div");
        empty.className = "empty-state";
        empty.textContent = "暂无通知";
        list.append(empty);
        return;
    }

    state.notifications.forEach(notification => {
        const row = document.createElement("article");
        row.className = "notification-card";
        const head = document.createElement("div");
        head.className = "item-card-head";
        const titleBox = document.createElement("div");
        const title = document.createElement("h3");
        title.textContent = notification.title || notification.noticeType;
        const meta = document.createElement("div");
        meta.className = "item-meta";
        const type = document.createElement("span");
        type.textContent = notification.noticeType || "-";
        const biz = document.createElement("span");
        biz.textContent = `${notification.bizType || "-"} / ${notification.bizId || "-"}`;
        const created = document.createElement("span");
        created.textContent = notification.createdAt || "-";
        meta.append(type, biz, created);
        titleBox.append(title, meta);
        const status = document.createElement("span");
        status.className = `item-status ${notification.readStatus === "UNREAD" ? "running" : "ready"}`;
        status.textContent = notification.readStatus;
        head.append(titleBox, status);

        const content = document.createElement("p");
        content.className = "notification-content";
        content.textContent = notification.content || "";

        const actions = document.createElement("div");
        actions.className = "item-card-actions";
        const readButton = document.createElement("button");
        readButton.className = "secondary-button";
        readButton.type = "button";
        readButton.textContent = "标记已读";
        readButton.disabled = notification.readStatus === "READ";
        readButton.addEventListener("click", () => markNotificationRead(notification.notificationId));
        actions.append(readButton);
        row.append(head, content, actions);
        list.append(row);
    });
}

function render() {
    const activeRoute = state.user ? state.route : "gate";
    $all("[data-view]").forEach(view => {
        view.hidden = view.dataset.view !== activeRoute;
    });
    renderNavigation();
    renderSession();
    if (state.user) {
        renderProfile();
    }
    if (activeRoute === "seller") {
        renderSeller();
    }
    if (activeRoute === "buyer") {
        renderBuyer();
    }
    if (activeRoute === "orders") {
        renderOrderPage();
    }
    if (activeRoute === "notifications") {
        renderNotifications();
    }
    if (activeRoute === "admin") {
        renderAdmin();
    }
}

async function request(path, options = {}) {
    const headers = {
        Accept: "application/json",
        ...(options.headers || {}),
    };

    if (state.token) {
        headers.Authorization = `Bearer ${state.token}`;
    }

    const response = await fetch(`${API_BASE}${path}`, {
        ...options,
        headers,
    });

    const contentType = response.headers.get("content-type") || "";
    const payload = contentType.includes("application/json")
        ? await response.json()
        : { code: response.status, message: await response.text(), data: {} };
    showResponse(payload);

    if (!response.ok || payload.code !== 0) {
        if (response.status === 401 || [4103, 4104, 4105, 4106, 4107].includes(payload.code)) {
            clearSession();
            setRoute("gate", { silent: true });
        }
        throw new Error(payload.message || `请求失败: ${response.status}`);
    }

    return payload;
}

function currentHashRoute() {
    return (window.location.hash || "").replace(/^#/, "") || "home";
}

async function loadCurrentUser(options = {}) {
    if (!state.token) {
        setRoute("gate", { silent: true });
        return null;
    }

    try {
        setStatus("刷新会话");
        const payload = await request("/auth/me");
        state.user = payload.data;
        const targetRoute = options.route || currentHashRoute();
        setRoute(canEnter(targetRoute) ? targetRoute : visibleRoutesForRole(state.user.roleCode)[0]);
        setStatus("会话有效", "success");
        return payload.data;
    } catch (error) {
        setStatus("会话失效", "error");
        if (!options.silent) {
            toast(error.message);
        }
        return null;
    }
}

async function login(event) {
    event.preventDefault();
    setStatus("登录中");

    try {
        const payload = await request("/auth/login", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                username: $("#usernameInput").value.trim(),
                password: $("#passwordInput").value,
            }),
        });

        storeSession(payload.data.token, {
            ...payload.data.userInfo,
            tokenExpireAt: payload.data.expireAt,
        });
        setRoute("home");
        setStatus("登录成功", "success");
        toast("登录成功");
    } catch (error) {
        setStatus("登录失败", "error");
        toast(error.message);
    }
}

async function register(event) {
    event.preventDefault();
    setStatus("注册中");

    try {
        const payload = await request("/auth/register", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                username: $("#registerUsernameInput").value.trim(),
                password: $("#registerPasswordInput").value,
                phone: $("#registerPhoneInput").value.trim(),
                email: $("#registerEmailInput").value.trim(),
                nickname: $("#registerNicknameInput").value.trim(),
            }),
        });

        $("#usernameInput").value = payload.data.username;
        $("#passwordInput").value = $("#registerPasswordInput").value;
        setStatus("注册成功", "success");
        toast("注册成功");
    } catch (error) {
        setStatus("注册失败", "error");
        toast(error.message);
    }
}

async function logout() {
    setStatus("登出中");
    try {
        if (state.token) {
            await request("/auth/logout", { method: "POST" });
        }
        clearSession();
        setRoute("gate", { silent: true });
        setStatus("已退出", "success");
        toast("已退出登录");
    } catch (error) {
        clearSession();
        setRoute("gate", { silent: true });
        setStatus("已清理本地会话", "success");
        toast(error.message);
    }
}

async function changeUserStatus(event) {
    event.preventDefault();
    const userId = $("#statusUserIdInput").value.trim();
    if (!userId) {
        toast("请输入目标用户 ID");
        return;
    }

    setStatus("更新状态");
    try {
        await request(`/admin/users/${encodeURIComponent(userId)}/status`, {
            method: "PATCH",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                status: $("#statusSelect").value,
                reason: $("#statusReasonInput").value.trim(),
            }),
        });
        setStatus("状态已更新", "success");
        toast("用户状态已更新");
    } catch (error) {
        setStatus("更新失败", "error");
        toast(error.message);
    }
}

async function loadMyItems(options = {}) {
    setStatus("加载拍品");
    const filter = $("#itemStatusFilter")?.value || "";
    const query = filter ? `?itemStatus=${encodeURIComponent(filter)}` : "";

    try {
        const payload = await request(`/items/mine${query}`);
        state.sellerItems = payload.data.list || [];
        state.sellerLoaded = true;
        renderSeller();
        setStatus("拍品已加载", "success");
        if (!options.silent) {
            toast("拍品已刷新");
        }
    } catch (error) {
        setStatus("加载失败", "error");
        toast(error.message);
    }
}

async function selectItem(itemId) {
    setStatus("读取拍品");
    try {
        const payload = await request(`/items/${encodeURIComponent(itemId)}`);
        state.selectedItem = payload.data;
        renderSeller();
        setStatus("拍品已选择", "success");
    } catch (error) {
        setStatus("读取失败", "error");
        toast(error.message);
    }
}

function readItemFormPayload() {
    const categoryId = Number($("#itemCategoryInput").value);
    const startPrice = Number($("#itemStartPriceInput").value);
    return {
        title: $("#itemTitleInput").value.trim(),
        description: $("#itemDescriptionInput").value.trim(),
        categoryId,
        startPrice,
        coverImageUrl: $("#itemCoverInput").value.trim(),
    };
}

async function saveItem(event) {
    event.preventDefault();
    const editId = $("#itemEditIdInput").value.trim();
    const payload = readItemFormPayload();

    setStatus(editId ? "更新拍品" : "创建拍品");
    try {
        const response = await request(editId ? `/items/${encodeURIComponent(editId)}` : "/items", {
            method: editId ? "PUT" : "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload),
        });
        const itemId = response.data.itemId;
        await loadMyItems({ silent: true });
        await selectItem(itemId);
        if (!editId) {
            resetItemForm();
            $("#imageItemIdInput").value = String(itemId);
        }
        setStatus(editId ? "拍品已更新" : "拍品已创建", "success");
        toast(editId ? "拍品已更新" : "拍品已创建");
    } catch (error) {
        setStatus("保存失败", "error");
        toast(error.message);
    }
}

async function addItemImage(event) {
    event.preventDefault();
    const itemId = $("#imageItemIdInput").value.trim();
    if (!itemId) {
        toast("请输入拍品 ID");
        return;
    }

    const sortNoValue = $("#imageSortInput").value.trim();
    const payload = {
        imageUrl: $("#imageUrlInput").value.trim(),
        isCover: $("#imageCoverInput").checked,
    };
    if (sortNoValue) {
        payload.sortNo = Number(sortNoValue);
    }

    setStatus("添加图片");
    try {
        await request(`/items/${encodeURIComponent(itemId)}/images`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload),
        });
        $("#imageUrlInput").value = "";
        $("#imageSortInput").value = "";
        $("#imageCoverInput").checked = false;
        await selectItem(itemId);
        await loadMyItems({ silent: true });
        setStatus("图片已添加", "success");
        toast("图片已添加");
    } catch (error) {
        setStatus("添加失败", "error");
        toast(error.message);
    }
}

async function deleteItemImage(imageId) {
    const itemId = state.selectedItem?.item?.itemId;
    if (!itemId) {
        return;
    }

    setStatus("删除图片");
    try {
        await request(`/items/${encodeURIComponent(itemId)}/images/${encodeURIComponent(imageId)}`, {
            method: "DELETE",
        });
        await selectItem(itemId);
        await loadMyItems({ silent: true });
        setStatus("图片已删除", "success");
        toast("图片已删除");
    } catch (error) {
        setStatus("删除失败", "error");
        toast(error.message);
    }
}

async function submitSelectedItem() {
    const itemId = state.selectedItem?.item?.itemId || $("#imageItemIdInput").value.trim();
    if (!itemId) {
        toast("请选择拍品");
        return;
    }

    setStatus("提交审核");
    try {
        await request(`/items/${encodeURIComponent(itemId)}/submit-audit`, {
            method: "POST",
        });
        await selectItem(itemId);
        await loadMyItems({ silent: true });
        setStatus("已提交审核", "success");
        toast("已提交审核");
    } catch (error) {
        setStatus("提交失败", "error");
        toast(error.message);
    }
}

async function loadPendingItems(options = {}) {
    setStatus("加载待审核");
    try {
        const payload = await request("/admin/items/pending");
        state.pendingItems = payload.data.list || [];
        renderAdmin();
        setStatus("待审核已加载", "success");
        if (!options.silent) {
            toast("待审核拍品已刷新");
        }
    } catch (error) {
        setStatus("加载失败", "error");
        toast(error.message);
    }
}

async function loadAuditLogs(itemId) {
    if (!itemId) {
        return;
    }
    setStatus("读取审核日志");
    try {
        const payload = await request(`/admin/items/${encodeURIComponent(itemId)}/audit-logs`);
        state.auditLogs = payload.data.list || [];
        renderAdmin();
        setStatus("审核日志已加载", "success");
    } catch (error) {
        setStatus("读取失败", "error");
        toast(error.message);
    }
}

async function submitAudit(event) {
    event.preventDefault();
    const itemId = $("#auditItemIdInput").value.trim();
    if (!itemId) {
        toast("请输入拍品 ID");
        return;
    }

    const auditStatus = $("#auditStatusSelect").value;
    setStatus("提交审核");
    try {
        await request(`/admin/items/${encodeURIComponent(itemId)}/audit`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                auditStatus,
                reason: $("#auditReasonInput").value.trim(),
            }),
        });
        if (auditStatus === "APPROVED") {
            $("#auctionItemIdInput").value = itemId;
        }
        await loadPendingItems({ silent: true });
        await loadAuditLogs(itemId);
        setStatus("审核已提交", "success");
        toast("审核已提交");
    } catch (error) {
        setStatus("审核失败", "error");
        toast(error.message);
    }
}

async function loadAdminAuctions(options = {}) {
    setStatus("加载拍卖");
    const filter = $("#auctionStatusFilter")?.value || "";
    const query = filter ? `?status=${encodeURIComponent(filter)}` : "";

    try {
        const payload = await request(`/admin/auctions${query}`);
        state.adminAuctions = payload.data.list || [];
        renderAdmin();
        setStatus("拍卖已加载", "success");
        if (!options.silent) {
            toast("拍卖已刷新");
        }
    } catch (error) {
        setStatus("加载失败", "error");
        toast(error.message);
    }
}

async function loadAdminDashboard(options = {}) {
    await Promise.all([
        loadPendingItems({ silent: true }),
        loadAdminAuctions({ silent: true }),
    ]);
    state.adminLoaded = true;
    if (!options.silent) {
        toast("后台数据已刷新");
    }
}

async function selectAuction(auctionId) {
    setStatus("读取拍卖");
    try {
        const payload = await request(`/admin/auctions/${encodeURIComponent(auctionId)}`);
        state.selectedAuction = payload.data;
        renderAdmin();
        setStatus("拍卖已选择", "success");
    } catch (error) {
        setStatus("读取失败", "error");
        toast(error.message);
    }
}

async function saveAuction(event) {
    event.preventDefault();
    const editId = $("#auctionEditIdInput").value.trim();
    const payload = readAuctionFormPayload();
    if (editId) {
        delete payload.itemId;
    }

    setStatus(editId ? "更新拍卖" : "创建拍卖");
    try {
        const response = await request(
            editId ? `/admin/auctions/${encodeURIComponent(editId)}` : "/admin/auctions",
            {
                method: editId ? "PUT" : "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(payload),
            }
        );
        const auctionId = response.data.auctionId;
        await loadAdminAuctions({ silent: true });
        await selectAuction(auctionId);
        if (!editId) {
            resetAuctionForm();
        }
        setStatus(editId ? "拍卖已更新" : "拍卖已创建", "success");
        toast(editId ? "拍卖已更新" : "拍卖已创建");
    } catch (error) {
        setStatus("保存失败", "error");
        toast(error.message);
    }
}

async function cancelAuction(auctionId) {
    const reason = window.prompt("取消原因", "admin cancel from app");
    if (reason === null) {
        return;
    }

    setStatus("取消拍卖");
    try {
        await request(`/admin/auctions/${encodeURIComponent(auctionId)}/cancel`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ reason }),
        });
        await loadAdminAuctions({ silent: true });
        await selectAuction(auctionId);
        setStatus("拍卖已取消", "success");
        toast("拍卖已取消");
    } catch (error) {
        setStatus("取消失败", "error");
        toast(error.message);
    }
}

async function loadPublicAuctions(options = {}) {
    setStatus("加载拍卖大厅");
    const params = new URLSearchParams();
    const keyword = $("#buyerKeywordInput")?.value.trim() || "";
    const status = $("#buyerStatusFilter")?.value || "";
    if (keyword) {
        params.set("keyword", keyword);
    }
    if (status) {
        params.set("status", status);
    }
    const query = params.toString() ? `?${params.toString()}` : "";

    try {
        const payload = await request(`/auctions${query}`);
        state.publicAuctions = payload.data.list || [];
        state.buyerLoaded = true;
        renderBuyer();
        setStatus("拍卖大厅已加载", "success");
        if (!options.silent) {
            toast("拍卖大厅已刷新");
        }
    } catch (error) {
        setStatus("加载失败", "error");
        toast(error.message);
    }
}

function mergeSelectedAuctionPrice(price) {
    if (!state.selectedPublicAuction || !price) {
        return;
    }
    state.selectedPublicAuction = {
        ...state.selectedPublicAuction,
        currentPrice: price.currentPrice,
        highestBidderMasked: price.highestBidderMasked,
        endTime: price.endTime,
        acceptingBids: price.acceptingBids,
    };
}

async function selectPublicAuction(auctionId) {
    setStatus("读取拍卖详情");
    try {
        const [detailPayload, pricePayload, bidsPayload, myBidPayload, notificationPayload] =
            await Promise.all([
                request(`/auctions/${encodeURIComponent(auctionId)}`),
                request(`/auctions/${encodeURIComponent(auctionId)}/price`),
                request(`/auctions/${encodeURIComponent(auctionId)}/bids?pageSize=10`),
                request(`/auctions/${encodeURIComponent(auctionId)}/my-bid`),
                request(
                    `/notifications?limit=10&bizType=AUCTION&bizId=${encodeURIComponent(auctionId)}`
                ),
            ]);
        state.selectedPublicAuction = detailPayload.data;
        state.selectedPublicPrice = pricePayload.data;
        state.selectedBidHistory = bidsPayload.data.list || [];
        state.selectedMyBidStatus = myBidPayload.data;
        state.selectedNotifications = notificationPayload.data.list || [];
        mergeSelectedAuctionPrice(state.selectedPublicPrice);
        renderBuyer();
        setStatus("拍卖详情已加载", "success");
    } catch (error) {
        setStatus("读取失败", "error");
        toast(error.message);
    }
}

async function refreshSelectedPublicAuction(options = {}) {
    const auctionId = state.selectedPublicAuction?.auctionId;
    if (!auctionId || refreshSelectedPublicAuction.inFlight) {
        return;
    }
    refreshSelectedPublicAuction.inFlight = true;
    try {
        const [pricePayload, bidsPayload, myBidPayload, notificationPayload] = await Promise.all([
            request(`/auctions/${encodeURIComponent(auctionId)}/price`),
            request(`/auctions/${encodeURIComponent(auctionId)}/bids?pageSize=10`),
            request(`/auctions/${encodeURIComponent(auctionId)}/my-bid`),
            request(
                `/notifications?limit=10&bizType=AUCTION&bizId=${encodeURIComponent(auctionId)}`
            ),
        ]);
        state.selectedPublicPrice = pricePayload.data;
        state.selectedBidHistory = bidsPayload.data.list || [];
        state.selectedMyBidStatus = myBidPayload.data;
        state.selectedNotifications = notificationPayload.data.list || [];
        mergeSelectedAuctionPrice(state.selectedPublicPrice);
        renderBuyer();
        if (!options.silent) {
            setStatus("价格已刷新", "success");
            toast("价格已刷新");
        }
    } catch (error) {
        if (!options.silent) {
            setStatus("刷新失败", "error");
            toast(error.message);
        }
    } finally {
        refreshSelectedPublicAuction.inFlight = false;
    }
}

function buildRequestId(prefix) {
    const random = Math.random().toString(36).slice(2, 10);
    return `${prefix}-${Date.now()}-${random}`;
}

async function submitPublicBid(event) {
    event.preventDefault();
    const auctionId = state.selectedPublicAuction?.auctionId;
    const input = $("#buyerBidAmountInput");
    if (!auctionId || !input) {
        return;
    }

    setStatus("提交出价");
    try {
        await request(`/auctions/${encodeURIComponent(auctionId)}/bids`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                requestId: buildRequestId(`web-bid-${auctionId}`),
                bidAmount: Number(input.value),
            }),
        });
        await refreshSelectedPublicAuction({ silent: true });
        await loadPublicAuctions({ silent: true });
        setStatus("出价成功", "success");
        toast("出价成功");
    } catch (error) {
        setStatus("出价失败", "error");
        toast(error.message);
    }
}

async function loadMyOrders(options = {}) {
    setStatus("加载订单");
    const params = new URLSearchParams();
    const role = $("#orderRoleFilter")?.value || "";
    const status = $("#orderStatusFilter")?.value || "";
    if (role) {
        params.set("role", role);
    }
    if (status) {
        params.set("status", status);
    }
    const query = params.toString() ? `?${params.toString()}` : "";

    try {
        const payload = await request(`/orders/mine${query}`);
        state.orders = payload.data.list || [];
        state.ordersLoaded = true;
        renderOrderPage();
        setStatus("订单已加载", "success");
        if (!options.silent) {
            toast("订单已刷新");
        }
    } catch (error) {
        setStatus("加载失败", "error");
        toast(error.message);
    }
}

async function selectOrder(orderId) {
    setStatus("读取订单");
    try {
        const [detailPayload, paymentPayload, reviewPayload] = await Promise.all([
            request(`/orders/${encodeURIComponent(orderId)}`),
            request(`/orders/${encodeURIComponent(orderId)}/payment`),
            request(`/orders/${encodeURIComponent(orderId)}/reviews`),
        ]);
        state.selectedOrder = detailPayload.data;
        state.selectedPaymentStatus = paymentPayload.data;
        state.selectedOrderReviews = reviewPayload.data;
        const targetUserId = reviewTargetId(state.selectedOrder);
        if (targetUserId) {
            const creditPayload = await request(`/users/${encodeURIComponent(targetUserId)}/reviews/summary`);
            state.selectedOrderCredit = creditPayload.data;
        } else {
            state.selectedOrderCredit = null;
        }
        renderOrderPage();
        setStatus("订单已选择", "success");
    } catch (error) {
        setStatus("读取失败", "error");
        toast(error.message);
    }
}

async function refreshSelectedOrder() {
    const orderId = selectedOrderId();
    if (!orderId) {
        return;
    }
    await selectOrder(orderId);
}

async function initiateOrderPayment(payChannel) {
    const orderId = selectedOrderId();
    if (!orderId) {
        return;
    }

    setStatus("发起支付");
    try {
        await request(`/orders/${encodeURIComponent(orderId)}/pay`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ payChannel }),
        });
        await refreshSelectedOrder();
        await loadMyOrders({ silent: true });
        setStatus("支付单已生成", "success");
        toast("支付单已生成");
    } catch (error) {
        setStatus("支付失败", "error");
        toast(error.message);
    }
}

async function shipSelectedOrder() {
    const orderId = selectedOrderId();
    if (!orderId) {
        return;
    }

    setStatus("确认发货");
    try {
        await request(`/orders/${encodeURIComponent(orderId)}/ship`, {
            method: "POST",
        });
        await refreshSelectedOrder();
        await loadMyOrders({ silent: true });
        setStatus("已发货", "success");
        toast("已发货");
    } catch (error) {
        setStatus("发货失败", "error");
        toast(error.message);
    }
}

async function confirmSelectedReceipt() {
    const orderId = selectedOrderId();
    if (!orderId) {
        return;
    }

    setStatus("确认收货");
    try {
        await request(`/orders/${encodeURIComponent(orderId)}/confirm-receipt`, {
            method: "POST",
        });
        await refreshSelectedOrder();
        await loadMyOrders({ silent: true });
        setStatus("已确认收货", "success");
        toast("已确认收货");
    } catch (error) {
        setStatus("收货失败", "error");
        toast(error.message);
    }
}

async function submitSelectedOrderReview(event) {
    event.preventDefault();
    const orderId = selectedOrderId();
    if (!orderId) {
        return;
    }

    setStatus("提交评价");
    try {
        await request("/reviews", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                orderId,
                rating: Number($("#reviewRatingSelect").value),
                content: $("#reviewContentInput").value.trim(),
            }),
        });
        await refreshSelectedOrder();
        await loadMyOrders({ silent: true });
        setStatus("评价已提交", "success");
        toast("评价已提交");
    } catch (error) {
        setStatus("评价失败", "error");
        toast(error.message);
    }
}

async function loadNotifications(options = {}) {
    setStatus("加载通知");
    const params = new URLSearchParams();
    const unreadOnly = $("#notificationUnreadOnlyInput")?.checked || false;
    const bizType = $("#notificationBizTypeFilter")?.value || "";
    if (unreadOnly) {
        params.set("unreadOnly", "true");
    }
    if (bizType) {
        params.set("bizType", bizType);
    }
    params.set("limit", "50");
    const query = `?${params.toString()}`;

    try {
        const payload = await request(`/notifications${query}`);
        state.notifications = payload.data.list || [];
        state.notificationsLoaded = true;
        renderNotifications();
        setStatus("通知已加载", "success");
        if (!options.silent) {
            toast("通知已刷新");
        }
    } catch (error) {
        setStatus("加载失败", "error");
        toast(error.message);
    }
}

async function markNotificationRead(notificationId) {
    if (!notificationId) {
        return;
    }

    setStatus("标记通知");
    try {
        await request(`/notifications/${encodeURIComponent(notificationId)}/read`, {
            method: "PATCH",
        });
        await loadNotifications({ silent: true });
        setStatus("通知已读", "success");
        toast("通知已标记已读");
    } catch (error) {
        setStatus("标记失败", "error");
        toast(error.message);
    }
}

async function verifyNotFound() {
    setStatus("验证 404");
    try {
        await request("/not-found");
        setStatus("异常响应", "error");
    } catch (error) {
        setStatus("404 已统一 JSON 化", "success");
        toast(error.message);
    }
}

function bindEvents() {
    $("#loginForm").addEventListener("submit", login);
    $("#registerForm").addEventListener("submit", register);
    $("#logoutButton").addEventListener("click", logout);
    $("#statusForm").addEventListener("submit", changeUserStatus);
    $("#buyerFilterForm").addEventListener("submit", event => {
        event.preventDefault();
        loadPublicAuctions();
    });
    $("#refreshBuyerAuctionsButton").addEventListener("click", () => loadPublicAuctions());
    $("#orderFilterForm").addEventListener("submit", event => {
        event.preventDefault();
        loadMyOrders();
    });
    $("#refreshOrdersButton").addEventListener("click", () => loadMyOrders());
    $("#orderRoleFilter").addEventListener("change", () => {
        state.ordersLoaded = false;
        loadMyOrders();
    });
    $("#orderStatusFilter").addEventListener("change", () => {
        state.ordersLoaded = false;
        loadMyOrders();
    });
    $("#notificationFilterForm").addEventListener("submit", event => {
        event.preventDefault();
        loadNotifications();
    });
    $("#refreshNotificationsButton").addEventListener("click", () => loadNotifications());
    $("#notificationUnreadOnlyInput").addEventListener("change", () => {
        state.notificationsLoaded = false;
        loadNotifications();
    });
    $("#notificationBizTypeFilter").addEventListener("change", () => {
        state.notificationsLoaded = false;
        loadNotifications();
    });
    $("#itemForm").addEventListener("submit", saveItem);
    $("#imageForm").addEventListener("submit", addItemImage);
    $("#resetItemFormButton").addEventListener("click", resetItemForm);
    $("#refreshItemsButton").addEventListener("click", () => loadMyItems());
    $("#itemStatusFilter").addEventListener("change", () => {
        state.sellerLoaded = false;
        loadMyItems();
    });
    $("#submitAuditButton").addEventListener("click", submitSelectedItem);
    $("#refreshPendingItemsButton").addEventListener("click", () => loadPendingItems());
    $("#auditForm").addEventListener("submit", submitAudit);
    $("#auctionForm").addEventListener("submit", saveAuction);
    $("#resetAuctionFormButton").addEventListener("click", resetAuctionForm);
    $("#refreshAuctionsButton").addEventListener("click", () => loadAdminAuctions());
    $("#auctionStatusFilter").addEventListener("change", () => {
        state.adminLoaded = false;
        loadAdminAuctions();
    });
    $("#contextButton").addEventListener("click", () => loadCurrentUser({ route: "api" }));
    $("#notFoundButton").addEventListener("click", verifyNotFound);

    $all("[data-route]").forEach(button => {
        button.addEventListener("click", () => setRoute(button.dataset.route));
    });
    $all("[data-route-jump]").forEach(button => {
        button.addEventListener("click", () => setRoute(button.dataset.routeJump));
    });
    window.addEventListener("hashchange", () => setRoute(currentHashRoute()));
}

async function boot() {
    renderEndpoints();
    bindEvents();
    window.setInterval(() => {
        if (state.route === "buyer" && state.user) {
            if (state.selectedPublicAuction) {
                refreshSelectedPublicAuction({ silent: true });
                return;
            }
            renderBuyer();
        }
    }, 10000);
    showResponse({ message: "等待首次真实接口调用" });
    render();

    if (state.token) {
        await loadCurrentUser({ route: currentHashRoute(), silent: true });
        return;
    }

    if (window.location.hash) {
        setRoute(currentHashRoute());
    } else {
        setRoute("gate", { silent: true });
    }
}

boot();
