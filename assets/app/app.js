const API_BASE = "/api";
const TOKEN_KEY = "auction_app_token";

const routeRoles = {
    gate: [],
    home: ["USER", "ADMIN", "SUPPORT"],
    buyer: ["USER"],
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
    { name: "上下文", path: "GET /api/system/context" },
];

const state = {
    token: window.sessionStorage.getItem(TOKEN_KEY) || "",
    user: null,
    route: "gate",
    sellerLoaded: false,
    sellerItems: [],
    selectedItem: null,
};

const statusClassMap = {
    REJECTED: "rejected",
    READY_FOR_AUCTION: "ready",
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
    state.sellerLoaded = false;
    state.sellerItems = [];
    state.selectedItem = null;
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
    $("#itemForm").addEventListener("submit", saveItem);
    $("#imageForm").addEventListener("submit", addItemImage);
    $("#resetItemFormButton").addEventListener("click", resetItemForm);
    $("#refreshItemsButton").addEventListener("click", () => loadMyItems());
    $("#itemStatusFilter").addEventListener("change", () => {
        state.sellerLoaded = false;
        loadMyItems();
    });
    $("#submitAuditButton").addEventListener("click", submitSelectedItem);
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
