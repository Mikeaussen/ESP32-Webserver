setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("anzahlCocktails").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/anzahlCocktails", true);
    xhttp.send();
}, 5000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("anzahlAlkoholfrei").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/anzahlAlkoholfrei", true);
    xhttp.send();
}, 5000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("anzahlLongdrinks").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/anzahlLongdrinks", true);
    xhttp.send();
}, 5000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("anzahlShots").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/anzahlShots", true);
    xhttp.send();
}, 5000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
           document.getElementById("anzahlSaft").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/anzahlSaft", true);
    xhttp.send();
}, 5000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("anzahlSonstige").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/anzahlSonstige", true);
    xhttp.send();
}, 5000);
