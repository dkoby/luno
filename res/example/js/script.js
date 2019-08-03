
/*
 *
 */
sendQuery = function(dst, data, callback, timeout, ext) {
    let responseType = (ext && ext.responseType) ? ext.responseType : "json";

    var reqListener = function() {
        if (this.readyState == 4) {
            if (this.status == 200)
            {
                let respTime = Date.now() - this.sendTime;
                if (callback)
                {
                    if (responseType == "json")
                    {
                        /* NOTE IE does not support responseType "json" */
                        let response = (typeof this.response === 'string') ? JSON.parse(this.response) : this.response
                        callback(response, respTime);
                    } else {
                        let response = {result: "ok", data: this.response};
                        callback(response, respTime);
                    }
                }
            } else {
                let response = {result: "error", status: this.status};
                if (callback)
                    callback(response);
            }
        }
    }
    var request = new XMLHttpRequest();
    request.onerror = function() {
        let response = {result: "error"};
        if (callback)
            callback(response);
        request.abort();
    }
    if (ext !== undefined && ext.useUploadProgress)
    {
        request.upload.onprogress = function(e) {
            let response = {result: "uploadProgress", ev: e}
            if (callback)
                callback(response);
        }
    }
    request.addEventListener("load", reqListener);

    let requestType  = (ext && ext.requestType) ? ext.requestType : "POST";

    request.open(requestType, dst);
    request.setRequestHeader("Content-type", "application/json");

    request.ontimeout = function() {
        let response = {result: "timeout"};
        if (callback)
            callback(response);
        request.abort();
    }
    request.timeout      = timeout;
    request.responseType = responseType;
    request.sendTime = Date.now();
    if (data)
    {
        let sendData;

        /* XXX IE can not send DataView */
        if (ArrayBuffer.isView(data))
            sendData = data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
        else
            sendData = data;
        request.send(sendData);
    } else {
        request.send();
    }

    return request;
}

sendQuery("json", JSON.stringify({
    value: 100,
}), function(response) {
    if (response.result == "ok") {
        let p = document.createElement("p");
        document.body.appendChild(p);
        p.innerHTML = "json response value: " + response.data.value;
    }
});

/*
 * Websocket test.
 */
if (true)
{
    let p = document.createElement("p");
    document.body.appendChild(p);

    let button = document.createElement("button");
    p.appendChild(button);
    button.innerHTML = "Websocket test";

    button.addEventListener("click", function() {
        console.log("click");
    });
}
/*
 * Database test.
 */
if (true) {
    let p = document.createElement("p");
    document.body.appendChild(p);

    if (true) {
        let div = document.createElement("div");
        p.appendChild(div);
        let button = document.createElement("button");
        div.appendChild(button);
        button.innerHTML = "Database store";

        button.addEventListener("click", function() {
            let data = JSON.stringify({
                value: input.value,
            });
            sendQuery("database?action=put", data, function(response) {
                if (response.result == "ok") {

                }
            });
        });

        let input = document.createElement("input");
        div.appendChild(input);
        input.setAttribute("type", "text");
    }

    if (true) {
        let div = document.createElement("div");
        p.appendChild(div);

        let list = document.createElement("div");

        let button = document.createElement("button");
        div.appendChild(button);
        div.appendChild(list);
        button.innerHTML = "Database get";

        button.addEventListener("click", function() {
            sendQuery("database?action=get", null, function(response) {
                if (response.result == "ok") {
                    while (list.hasChildNodes())
                        list.removeChild(list.firstChild);
                    response.data.forEach(function(data) {
                        let item = document.createElement("div");
                        item.innerHTML = data.content;
                        list.appendChild(item);
                    });
                }
            });
        });
    }
}

console.log("COOKIE", document.cookie)

//
//
//if (true)
//{
//    let button = document.getElementById("testButton");
//    let callback = function(response) {
//        console.log("RESPONSE", response);
//    }
//    let click = function(ev) {
//        let json = {
//            "Key1": [
//                1234, 5678, 91012
//            ],
//            "Key2": {
//                "Subkey1" : true,
//                "Subkey2" : 1e12,
//                "Subkey3" : [1, 2, "testString"],
//            },
//        }
//        Test.sendQuery("/test", JSON.stringify(json), "json", callback, 1000);
////        Test.sendQuery("/test", JSON.stringify(json), "base64", callback, 1000);
//    }
//    button.addEventListener("click", click);
//}
//
//if (true)
//{
//    let button = document.getElementById("testWSButton");
//    let click = function(ev) {
////        new MyWSConnection("ws:/127.0.0.1:8080/websock");
//        new MyWSConnection("wss:/127.0.0.1:8443/websock");
//    }
//    button.addEventListener("click", click);
//}
//
//var MyWSConnection = function(path)
//{
//    var wsocket = new WebSocket(path);
//
//    let sendFunction = function(i) {
//        if (i === undefined)
//            i = 0;
//
//        if (i == 200)
//        {
//            wsocket.close();
//            return;
//        }
//
//        wsocket.send("Test Тест " + i);
//        window.setTimeout(sendFunction.bind(this, i + 1), 50);
//    }
//
//    let onopen = function(ev) {
//        console.log("onOpen");
////        let DATA_SIZE = 123456
////        let data = new DataView(new ArrayBuffer(DATA_SIZE));
////        for (let i = 0; i < DATA_SIZE; i++)
////            data.setUint8(i, i);
////        wsocket.send(data);
////        wsocket.send("Test");
////        wsocket.close();
//        sendFunction();
//    }
//    let onmessage = function(ev) {
//       console.log("onMessage: \"" + ev.data + "\"");
//    }
//    let onerror = function(ev) {
//        console.log("onError");
//    }
//    let onclose = function(ev) {
//        console.log("onClose");
//    }
//
//    wsocket.onopen    = onopen.bind(this);
//    wsocket.onmessage = onmessage.bind(this);
//    wsocket.onerror   = onerror.bind(this);
//    wsocket.onclose   = onclose.bind(this);
//
////    wsocket.close();
//}
//
