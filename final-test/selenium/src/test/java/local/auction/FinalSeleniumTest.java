package local.auction;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.math.BigDecimal;
import java.net.URI;
import java.net.URLEncoder;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.logging.Level;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.TestInfo;
import org.openqa.selenium.By;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.Keys;
import org.openqa.selenium.OutputType;
import org.openqa.selenium.TakesScreenshot;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.chrome.ChromeDriver;
import org.openqa.selenium.chrome.ChromeOptions;
import org.openqa.selenium.logging.LogEntry;
import org.openqa.selenium.logging.LogType;
import org.openqa.selenium.logging.LoggingPreferences;
import org.openqa.selenium.support.ui.ExpectedConditions;
import org.openqa.selenium.support.ui.WebDriverWait;

class FinalSeleniumTest {
    private static final String FRONTEND_BASE = property("auction.frontend.url", "http://127.0.0.1:3100");
    private static final String API_BASE = property("auction.api.url", "http://127.0.0.1:18080");
    private static final String RUN_ID = property("auction.run.id", "selenium-local");
    private static final Path ARTIFACT_DIR = Path.of(property("auction.artifact.dir", "target/selenium-artifacts"));

    private static TestApi api;
    private static MysqlCli mysql;

    private WebDriver driver;
    private WebDriverWait wait;

    @BeforeAll
    static void beforeAll() throws Exception {
        Files.createDirectories(ARTIFACT_DIR.resolve("screenshots"));
        Files.createDirectories(ARTIFACT_DIR.resolve("console"));
        api = new TestApi(API_BASE);
        mysql = new MysqlCli();
        api.expectHealthz();
    }

    @BeforeEach
    void beforeEach() {
        LoggingPreferences logging = new LoggingPreferences();
        logging.enable(LogType.BROWSER, Level.ALL);

        ChromeOptions options = new ChromeOptions();
        propertyOptional("auction.chrome.binary").ifPresent(options::setBinary);
        options.addArguments(
            "--headless=new",
            "--no-sandbox",
            "--disable-dev-shm-usage",
            "--disable-gpu",
            "--window-size=1440,1000",
            "--lang=zh-CN"
        );
        options.setCapability("goog:loggingPrefs", logging);

        driver = new ChromeDriver(options);
        wait = new WebDriverWait(driver, Duration.ofSeconds(20));
    }

    @AfterEach
    void afterEach(TestInfo info) throws Exception {
        if (driver == null) {
            return;
        }

        String testName = info.getTestMethod().map(method -> method.getName()).orElse("unknown");
        Path screenshot = ARTIFACT_DIR.resolve("screenshots").resolve(testName + ".png");
        Files.write(screenshot, ((TakesScreenshot) driver).getScreenshotAs(OutputType.BYTES));

        Path console = ARTIFACT_DIR.resolve("console").resolve(testName + ".log");
        List<String> lines = new ArrayList<>();
        for (LogEntry entry : driver.manage().logs().get(LogType.BROWSER)) {
            lines.add(entry.getLevel() + " " + entry.getTimestamp() + " " + entry.getMessage());
        }
        Files.write(console, lines, StandardCharsets.UTF_8);

        driver.quit();
    }

    @Test
    void authenticationSessionAndRbacProtection() {
        loginViaUi("buyer_demo", "Buyer@123");
        driver.navigate().refresh();
        waitForText("拍卖大厅");
        waitForText("演示买家");

        driver.get(FRONTEND_BASE + "/admin/dashboard");
        wait.until(ExpectedConditions.or(
            ExpectedConditions.urlToBe(FRONTEND_BASE + "/"),
            ExpectedConditions.urlContains(FRONTEND_BASE + "/?")
        ));
        assertFalse(driver.getPageSource().contains("审核与运维大盘"), "普通用户不应看到管理员大盘");

        ((JavascriptExecutor) driver).executeScript("window.sessionStorage.clear();");
        driver.get(FRONTEND_BASE + "/notifications");
        wait.until(ExpectedConditions.urlContains("/auth/login"));
        waitForText("登录 / 注册");
    }

    @Test
    void notificationReadRegression() throws Exception {
        String title = "Selenium 通知已读 " + RUN_ID;
        long notificationId = mysql.insertNotificationForUser(
            "buyer_demo",
            "SYSTEM_NOTICE",
            title,
            "用于 JUnit Selenium 验证通知已读链路",
            "AUCTION",
            9000001L
        );

        loginViaUi("buyer_demo", "Buyer@123");
        driver.get(FRONTEND_BASE + "/notifications");
        WebElement article = wait.until(ExpectedConditions.visibilityOfElementLocated(
            By.xpath("//article[.//*[contains(normalize-space(.), '" + title + "')]]")
        ));
        assertTrue(article.getText().contains("未读"), "前置通知应为未读");
        assertFalse(driver.getPageSource().contains("无法连接后端"), "通知页不应出现后端连接错误");

        article.findElement(By.xpath(".//button[contains(normalize-space(.), '已读')]")).click();
        waitForText("已标记为已读");
        wait.until(d -> d.getPageSource().contains("已读") && !d.getPageSource().contains("无法连接后端"));

        String buyerToken = api.login("buyer_demo", "Buyer@123");
        String response = api.get("/api/notifications?limit=20", buyerToken);
        assertTrue(response.contains("\"notificationId\":" + notificationId), "通知列表应包含本次通知");
        assertTrue(response.contains("\"readStatus\":\"READ\""), "后端事实应已标记为 READ");
    }

    @Test
    void adminStatisticsDashboardRegression() throws Exception {
        LocalDate today = LocalDate.now();
        mysql.upsertDailyStatistics(today, 9, 7, 2, 31, new BigDecimal("1234.56"));

        loginViaUi("admin", "Admin@123");
        driver.get(FRONTEND_BASE + "/admin/dashboard");
        waitForText("审核与运维大盘");
        clickButton("统计报表");
        waitForText("今日成交");
        waitForText("今日 GMV");
        waitForText("7");
        wait.until(d -> d.getPageSource().contains("1,235"));

        String adminToken = api.login("admin", "Admin@123");
        String encodedDate = URLEncoder.encode(today.toString(), StandardCharsets.UTF_8);
        String response = api.get(
            "/api/admin/statistics/daily?startDate=" + encodedDate + "&endDate=" + encodedDate,
            adminToken
        );
        assertTrue(response.contains("\"soldCount\":7") || response.contains("\"sold_count\":7"));
        assertTrue(response.contains("\"gmvAmount\":") || response.contains("\"gmv_amount\":"));
        assertFalse(driver.getPageSource().contains("无法连接后端"), "统计页不应出现后端连接错误");
    }

    @Test
    void sellerPublishAdminApproveAndCreateAuctionFlow() {
        String titleNonce = String.valueOf(System.currentTimeMillis());
        String title = "Selenium 建拍闭环 " + RUN_ID + " " + titleNonce;
        String description = "JUnit Selenium 真实前后端发布审核建拍链路。";
        String imageUrl = FRONTEND_BASE + "/demo-auction.svg";

        loginViaUi("seller_demo", "Seller@123");
        ((JavascriptExecutor) driver).executeScript("window.localStorage.removeItem('auction-publish-draft');");
        driver.get(FRONTEND_BASE + "/auction/publish");
        waitForText("拍品发布");

        fillFieldAfterLabel("拍品标题", "input", title);
        fillFieldAfterLabel("拍品描述", "textarea", description);
        clickButton("下一步");
        waitForText("粘贴图片 URL");
        wait.until(ExpectedConditions.elementToBeClickable(By.cssSelector("input[placeholder^='https://']"))).sendKeys(imageUrl);
        clickButton("添加图片");
        waitForText("图片已添加");
        clickButton("下一步");
        fillFieldAfterLabel("起拍价", "input", "320");
        fillFieldAfterLabel("建议最小加价幅度", "input", "20");
        fillFieldAfterLabel("交付地点", "input", "犀浦校区 Selenium 验证点");
        clickButton("下一步");
        waitForText("提交后进入管理员审核队列");
        clickExactButton("提交审核");
        waitForText("提交成功");

        clearBrowserSession();
        loginViaUi("admin", "Admin@123");
        driver.get(FRONTEND_BASE + "/admin/dashboard");
        waitForText("待审核拍品");
        waitForText(title);
        driver.findElement(By.xpath("//tr[.//td[contains(normalize-space(.), '" + title + "')]]")).click();
        waitForText("REVIEW DRAWER");
        clickButton("批准");
        waitForText("已批准");
        clickButton("创建拍卖");
        waitForText("确认创建拍卖");
        clickButton("确认创建拍卖");
        waitForText("已创建拍卖");
        String createdAuctionId = firstBodyMatch("已创建拍卖\\s*#(\\d+)");
        clickButton("拍卖监控");
        waitForText(title);

        String adminToken = api.login("admin", "Admin@123");
        String auctionDetail = api.get("/api/admin/auctions/" + createdAuctionId, adminToken);
        assertTrue(
            auctionDetail.contains(titleNonce),
            "管理端拍卖详情应包含 Selenium 创建的拍卖，auctionId=" + createdAuctionId +
                ", response=" + excerpt(auctionDetail)
        );
        assertTrue(auctionDetail.contains("\"auctionId\":" + createdAuctionId), "详情 auctionId 应匹配本次创建结果");
        assertTrue(auctionDetail.contains("\"status\":\"PENDING_START\""), "新建拍卖应处于 PENDING_START");
    }

    private void loginViaUi(String username, String password) {
        clearBrowserSession();
        driver.get(FRONTEND_BASE + "/auth/login");
        wait.until(ExpectedConditions.visibilityOfElementLocated(By.cssSelector("input[name='username']"))).sendKeys(username);
        driver.findElement(By.cssSelector("input[name='password']")).sendKeys(password);
        clickButton("进入拍卖大厅");
        wait.until(ExpectedConditions.urlContains("/auction/hall"));
        waitForText("拍卖大厅");
    }

    private void clearBrowserSession() {
        driver.get(FRONTEND_BASE + "/auth/login");
        ((JavascriptExecutor) driver).executeScript(
            "window.sessionStorage.clear(); window.localStorage.removeItem('auction-publish-draft');"
        );
    }

    private void fillFieldAfterLabel(String label, String tag, String value) {
        WebElement field = wait.until(ExpectedConditions.elementToBeClickable(By.xpath(
            "//label[.//*[normalize-space()='" + label + "'] or contains(normalize-space(.), '" + label + "')]//" + tag + "[1]"
        )));
        field.sendKeys(Keys.chord(Keys.CONTROL, "a"));
        field.sendKeys(value);
    }

    private void clickButton(String text) {
        WebElement button = wait.until(ExpectedConditions.elementToBeClickable(By.xpath(
            "//button[contains(normalize-space(.), '" + text + "')]"
        )));
        ((JavascriptExecutor) driver).executeScript("arguments[0].scrollIntoView({block: 'center'});", button);
        button.click();
    }

    private void clickExactButton(String text) {
        WebElement button = wait.until(ExpectedConditions.elementToBeClickable(By.xpath(
            "//button[normalize-space(.)='" + text + "']"
        )));
        ((JavascriptExecutor) driver).executeScript("arguments[0].scrollIntoView({block: 'center'});", button);
        button.click();
    }

    private void waitForText(String text) {
        wait.until(ExpectedConditions.textToBePresentInElementLocated(By.tagName("body"), text));
    }

    private String firstBodyMatch(String regex) {
        String text = driver.findElement(By.tagName("body")).getText();
        Matcher matcher = Pattern.compile(regex).matcher(text);
        assertTrue(matcher.find(), "页面正文应匹配 " + regex + "，body=" + excerpt(compact(text)));
        return matcher.group(1);
    }

    private static String property(String key, String fallback) {
        String value = System.getProperty(key);
        return value == null || value.isBlank() ? fallback : value;
    }

    private static Optional<String> propertyOptional(String key) {
        String value = System.getProperty(key);
        return value == null || value.isBlank() ? Optional.empty() : Optional.of(value);
    }

    private static String excerpt(String value) {
        return value.length() <= 500 ? value : value.substring(0, 500) + "...";
    }

    private static final class TestApi {
        private final HttpClient client = HttpClient.newBuilder()
            .connectTimeout(Duration.ofSeconds(5))
            .build();
        private final String baseUrl;

        TestApi(String baseUrl) {
            this.baseUrl = baseUrl;
        }

        void expectHealthz() throws Exception {
            HttpRequest request = HttpRequest.newBuilder(URI.create(baseUrl + "/healthz"))
                .timeout(Duration.ofSeconds(5))
                .GET()
                .build();
            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
            assertTrue(response.statusCode() == 200, "healthz should return 200 but got " + response.statusCode());
        }

        String login(String username, String password) {
            String body = "{\"username\":\"" + jsonEscape(username) + "\",\"password\":\"" + jsonEscape(password) + "\"}";
            String response = post("/api/auth/login", body, null);
            return extractString(response, "token");
        }

        String get(String path, String token) {
            try {
                HttpRequest.Builder builder = HttpRequest.newBuilder(URI.create(baseUrl + path))
                    .timeout(Duration.ofSeconds(10))
                    .header("Accept", "application/json")
                    .GET();
                if (token != null && !token.isBlank()) {
                    builder.header("Authorization", "Bearer " + token);
                }
                HttpResponse<String> response = client.send(builder.build(), HttpResponse.BodyHandlers.ofString());
                assertTrue(
                    response.statusCode() >= 200 && response.statusCode() < 300,
                    "GET " + path + " failed with " + response.statusCode() + ": " + response.body()
                );
                return response.body();
            } catch (IOException exception) {
                throw new AssertionError("GET " + path + " failed", exception);
            } catch (InterruptedException exception) {
                Thread.currentThread().interrupt();
                throw new AssertionError("GET " + path + " interrupted", exception);
            }
        }

        private String post(String path, String body, String token) {
            try {
                HttpRequest.Builder builder = HttpRequest.newBuilder(URI.create(baseUrl + path))
                    .timeout(Duration.ofSeconds(10))
                    .header("Accept", "application/json")
                    .header("Content-Type", "application/json")
                    .POST(HttpRequest.BodyPublishers.ofString(body, StandardCharsets.UTF_8));
                if (token != null && !token.isBlank()) {
                    builder.header("Authorization", "Bearer " + token);
                }
                HttpResponse<String> response = client.send(builder.build(), HttpResponse.BodyHandlers.ofString());
                assertTrue(
                    response.statusCode() >= 200 && response.statusCode() < 300,
                    "POST " + path + " failed with " + response.statusCode() + ": " + response.body()
                );
                return response.body();
            } catch (IOException exception) {
                throw new AssertionError("POST " + path + " failed", exception);
            } catch (InterruptedException exception) {
                Thread.currentThread().interrupt();
                throw new AssertionError("POST " + path + " interrupted", exception);
            }
        }
    }

    private static final class MysqlCli {
        private final String mode = env("SELENIUM_MYSQL_MODE", "socket");
        private final String socket = env("SELENIUM_MYSQL_SOCKET", "");
        private final String host = env("SELENIUM_MYSQL_HOST", "127.0.0.1");
        private final String port = env("SELENIUM_MYSQL_PORT", "3406");
        private final String database = env("SELENIUM_MYSQL_DATABASE", "auction_system");
        private final String user = env("SELENIUM_MYSQL_USER", "auction_user");
        private final String password = env("SELENIUM_MYSQL_PASSWORD", "change_me");

        long insertNotificationForUser(
            String username,
            String noticeType,
            String title,
            String content,
            String bizType,
            long bizId
        ) throws Exception {
            String userId = scalar("SELECT user_id FROM user_account WHERE username = '" + sqlEscape(username) + "' LIMIT 1");
            return Long.parseLong(scalar(
                "INSERT INTO notification (user_id, notice_type, title, content, biz_type, biz_id, read_status, push_status) " +
                "VALUES (" + userId + ", '" + sqlEscape(noticeType) + "', '" + sqlEscape(title) + "', '" +
                sqlEscape(content) + "', '" + sqlEscape(bizType) + "', " + bizId + ", 'UNREAD', 'SENT'); " +
                "SELECT LAST_INSERT_ID();"
            ));
        }

        void upsertDailyStatistics(
            LocalDate date,
            int auctionCount,
            int soldCount,
            int unsoldCount,
            int bidCount,
            BigDecimal gmvAmount
        ) throws Exception {
            runSql(
                "INSERT INTO statistics_daily (stat_date, auction_count, sold_count, unsold_count, bid_count, gmv_amount) " +
                "VALUES ('" + date + "', " + auctionCount + ", " + soldCount + ", " + unsoldCount + ", " +
                bidCount + ", " + gmvAmount + ") " +
                "ON DUPLICATE KEY UPDATE auction_count = VALUES(auction_count), sold_count = VALUES(sold_count), " +
                "unsold_count = VALUES(unsold_count), bid_count = VALUES(bid_count), gmv_amount = VALUES(gmv_amount), " +
                "updated_at = CURRENT_TIMESTAMP(3);"
            );
        }

        private String scalar(String sql) throws Exception {
            List<String> command = baseCommand();
            command.add("-Nse");
            command.add(sql);
            command.add(database);
            ProcessResult result = runProcess(command, null);
            assertTrue(result.exitCode == 0, "mysql scalar failed: " + result.output);
            return result.output.trim();
        }

        private void runSql(String sql) throws Exception {
            List<String> command = baseCommand();
            command.add(database);
            ProcessResult result = runProcess(command, sql);
            assertTrue(result.exitCode == 0, "mysql sql failed: " + result.output);
        }

        private List<String> baseCommand() {
            List<String> command = new ArrayList<>();
            command.add("mysql");
            if ("socket".equals(mode) && !socket.isBlank()) {
                command.add("--socket=" + socket);
            } else {
                command.add("-h" + host);
                command.add("-P" + port);
            }
            command.add("-u" + user);
            return command;
        }
    }

    private static ProcessResult runProcess(List<String> command, String input) throws Exception {
        ProcessBuilder builder = new ProcessBuilder(command);
        String mysqlPassword = System.getenv("SELENIUM_MYSQL_PASSWORD");
        if (mysqlPassword != null && !mysqlPassword.isBlank()) {
            builder.environment().put("MYSQL_PWD", mysqlPassword);
        }
        builder.redirectErrorStream(true);
        Process process = builder.start();
        if (input != null) {
            try (OutputStream out = process.getOutputStream()) {
                out.write(input.getBytes(StandardCharsets.UTF_8));
            }
        } else {
            process.getOutputStream().close();
        }
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        process.getInputStream().transferTo(output);
        int exitCode = process.waitFor();
        return new ProcessResult(exitCode, output.toString(StandardCharsets.UTF_8));
    }

    private record ProcessResult(int exitCode, String output) {}

    private static String env(String key, String fallback) {
        String value = System.getenv(key);
        return value == null || value.isBlank() ? fallback : value;
    }

    private static String compact(String value) {
        return value.replaceAll("\\s+", "");
    }

    private static String extractString(String json, String field) {
        Matcher matcher = Pattern.compile("\"" + Pattern.quote(field) + "\"\\s*:\\s*\"([^\"]*)\"").matcher(json);
        assertTrue(matcher.find(), "missing JSON field: " + field + " in " + json);
        return matcher.group(1);
    }

    private static String jsonEscape(String value) {
        return value.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    private static String sqlEscape(String value) {
        return value.replace("\\", "\\\\").replace("'", "''");
    }
}
