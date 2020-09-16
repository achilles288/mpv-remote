var loaderBrowserWindow;
var loaderURLWindow;
var loaderFileInput;
var loaderStatus;
var loaderTimer;
var loaderStartTime;
var loaderLoading = false;




function onLoaderLoaded(data) {
  loaderStatus.innerHTML = "";
  window.clearInterval(loaderTimer);
  loaderLoading = false;
  
  var name = document.getElementById("media-name");
  name.innerHTML = (data.name.trim() != "") ? data.name : "Untitled";
  name.style.color = "inherit";
  
  var controllerButtons
    = document.getElementById("media-controller-buttons").children;
  
  for(var i=0; i<controllerButtons.length; i++)
    controllerButtons[i].classList.remove("disabled");
  
  var tools = document.getElementById("media-controller-tools");
  var timeLabel = document.getElementById("time-label");
  tools.style.display = "block";
  var hr = Math.floor(data.duration / 3600);
  var min = Math.floor(data.duration / 60) % 60;
  var sec = Math.floor(data.duration) % 60;
  hr = String(hr);
  min = String(min).padStart(2, "0");
  sec = String(sec).padStart(2, "0");
  timeLabel.innerHTML = "0:00:00 / " + hr + ":" + min + ":" + sec;
}




function onLoaderIdle(event) {
  var t = new Date().getTime();
  if(t - loaderStartTime > 31000) {
    loaderStatus.innerHTML = "Sever not responding";
    window.clearInterval(loaderTimer);
    loaderLoading = false;
    return;
  }
  
  fetch(
    "status",
    {
      method: "GET",
      credentials: "same-origin",
      headers: {"Accepts":"application/json"}
    }
  )
    .then(response => {
      if(response.status == 200)
        return Promise.resolve(response.json());
      else
        return Promise.reject(new Error("Error syncing"));
    })
    .then(data => {
      if(data.loaded)
        onLoaderLoaded(data);
      
      if(data.error.code != 0) {
        loaderStatus.innerHTML = data.error.message;
        window.clearInterval(loaderTimer);
        loaderLoading = false;
      }
    })
    .catch((error) => console.error(error));
}




function loaderLoad(win, url) {
  if(loaderLoading)
    return;
  
  var formData = new FormData();
  formData.append("command", 'open "' + url + '" --pause');
  
  fetch(
    "command",
    {
      method: "POST",
      body: formData,
      credentials: "same-origin"
    }
  )
    .then(response => {
      if(response.status == 200) {
        win.getElementsByClassName("error")[0].innerHTML = "";
        win.parentElement.style.display = "none";
        loaderStartTime = new Date().getTime();
        loaderLoading = true;
        loaderTimer = window.setInterval(onLoaderIdle, 100);
      }
      else if(response.status == 401) {
        win.getElementsByClassName("error")[0].innerHTML
          = "Error 401 (Unauthorized)";
      }
      else {
        win.getElementsByClassName("error")[0].innerHTML = "Error";
      }
    })
    .catch((error) => console.error(error));
}




function onLoaderBrowse(event) {
  
}

function onLoaderBrowserSubmit(event) {
  
}




function onLoaderURL(event) {
  if(loaderLoading == false)
    loaderURLWindow.parentElement.style.display = "block";
}

function onLoaderURLSubmit(event) {
  event.preventDefault();
  var url = document.getElementById("media-loader-url-input").value;
  loaderLoad(event.target, url);
}




function onLoaderUpload(event) {
  var formData = new FormData();
  formData.append("blob", loaderFileInput.files[0]);
  
  fetch(
    "upload",
    {
      method: "POST",
      body: formData,
      credentials: "same-origin",
      headers: {"Accepts":"application/json"}
    }
  )
    .then(response => {
      if(response.status == 200)
        return Promise.resolve(response.json());
      else
        return Promise.reject(new Error("Error uploading"));
    })
    .then(data => {
      loaderLoad(data.url);
    })
    .catch((error) => console.error(error));
}




function loaderCloseWindow(event) {
  event.target.parentElement.parentElement.style.display = "none";
}




window.addEventListener(
  "load",
  function() {
    var buttons = document.getElementById("media-loader-buttons").children;
    buttons[0].onclick = onLoaderBrowse;
    buttons[1].onclick = onLoaderUpload;
    buttons[2].onclick = onLoaderURL;
    
    loaderBrowserWindow = document.getElementById("media-loader-browser-win");
    loaderURLWindow = document.getElementById("media-loader-url-win");
    loaderStatus = document.getElementById("media-loader-status");
    
    loaderBrowserWindow.onsubmit = onLoaderBrowserSubmit;
    loaderURLWindow.onsubmit = onLoaderURLSubmit;
    
    loaderBrowserWindow.getElementsByClassName("close")[0].onclick
      = loaderCloseWindow;
    
    loaderURLWindow.getElementsByClassName("close")[0].onclick
      = loaderCloseWindow;
    
    loaderFileInput = buttons[1].getElementsByTagName("input")[0];
    
    buttons[1].onclick = function(event) {
      if(loaderLoading)
        return;
      var clickEvent = new MouseEvent('click');
      loaderFileInput.dispatchEvent(clickEvent);
    };
  }
);
