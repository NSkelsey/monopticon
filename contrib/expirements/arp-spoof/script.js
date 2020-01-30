if (document.URL.endsWith("report.html")) {
    console.log('y');
    setInterval(function() {
        var b = document.querySelector("body");
        b.setAttribute("style", "background-color: red;");
        b.innerHTML = "<html><body style='background-color: red; width:100%; height: 100%;'><h1 style='color: white; font-size: 5em; padding-top: 200px; text-align:center; margin: auto;'>MiTM</h1></body></html>";
    }, 1000);
}
