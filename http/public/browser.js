var browserWindow;
var browserDirectoryLabel;
var browserPanel;
var browserDirectory = "";
var browserSelectedItem = null;
var browserFormInput;




function browserOpen(event) {
  browserSetHome();
  browserWindow.parentElement.style.display = "block";
}

function browserSubmit() {
  let formEvent = new Event("submit", {
    bubbles: true,
    cancelable: true,
  });
  browserWindow.dispatchEvent(formEvent);
}




function browserSetHome() {
  while(browserPanel.firstChild) {
    browserPanel.firstChild.remove();
  }
  browserDirectory = "";
  browserDirectoryLabel.innerHTML
    = "<i class='fas fa-folder'></i> " + browserDirectory;
  browserSelectedItem = null;
  browserFormInput.value = "";
  
  const folders = [
    {name: "Uploads", path: "uploads", image: "images/folder-dropbox.png"},
    {name: "Videos", path: "${Videos}", image: "images/folder-videos.png"},
    {name: "Music", path: "${Music}", image: "images/folder-music.png"}
  ];

  for(let i=0; i<folders.length; i++) {
    let elem, img, label, hiddenLabel;
    elem = document.createElement("div");
    elem.classList.add("browser-item");
    img = document.createElement("img");
    img.src = folders[i].image;
    label = document.createElement("p");
    label.innerHTML = folders[i].name;
    hiddenLabel = document.createElement("p");
    hiddenLabel.innerHTML = folders[i].path;
    hiddenLabel.style.display = "none";
    elem.appendChild(img);
    elem.appendChild(hiddenLabel);
    elem.appendChild(label);
    elem.onclick = function(event) { browserSelectDirectory(this); };
    elem.ondblclick = function(event) { browserOpenDirectory(this); };
    browserPanel.appendChild(elem);
  }

  fetch(
    "browse",
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
        return Promise.reject(new Error("Error browsing"));
    })
    .then(data => {
      let files = data.files;
      for(let i=0; i<files.length; i++) {
        let elem, img, label;
        elem = document.createElement("div");
        elem.classList.add("browser-item");
        img = document.createElement("img");
        img.src = "images/drive.png";
        label = document.createElement("p");
        label.innerHTML = files[i].name;
        elem.appendChild(img);
        elem.appendChild(label);
        elem.onclick = function(event) { browserSelectDirectory(this); };
        elem.ondblclick = function(event) { browserOpenDirectory(this); };
        browserPanel.appendChild(elem);
      }
    })
    .catch((error) => console.error(error));
}




function browserBrowse(path) {
  fetch(
    "browse?path=" + path,
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
        return Promise.reject(new Error("Error browsing"));
    })
    .then(data => {
      while(browserPanel.firstChild) {
        browserPanel.firstChild.remove();
      }
      browserDirectory = path;
      browserDirectoryLabel.innerHTML
        = "<i class='fas fa-folder'></i> " + browserDirectory;
      browserSelectedItem = null;
      browserFormInput.value = "";
      
      let files = data.files;
      for(let i=0; i<files.length; i++) {
        let name = files[i].name;
        let type = files[i].type;
        let elem, img, label;
        elem = document.createElement("div");
        elem.classList.add("browser-item");
        img = document.createElement("img");
        label = document.createElement("p");
        label.innerHTML = name;
        if(type == "directory") {
          img.src = "images/folder.png";
          elem.onclick = function(event) { browserSelectDirectory(this); };
          elem.ondblclick = function(event) { browserOpenDirectory(this); };
        }
        else {
          if(type == "video") {
            img.src = "images/file-video.png";
            let thumb = document.createElement("img");
            thumb.src = "thumbnail?file=" + browserDirectory + "/" + name;
            thumb.classList.add("thumbnail");
            thumb.onload = ((event) => event.target.nextSibling.remove());
            thumb.onerror = ((event) => event.target.remove());
            elem.appendChild(thumb);
          }
          else if(type == "audio")
            img.src = "images/file-audio.png";
          elem.onclick = function(event) { browserSelectItem(this); };
          elem.ondblclick = ((event) => browserSubmit());
        }
        elem.appendChild(img);
        elem.appendChild(label);
        browserPanel.appendChild(elem);
      }
    })
    .catch((error) => console.error(error));
}




function browserOnUpNavigate(event) {
  if(browserDirectory == "")
    return;
  let i = browserDirectory.lastIndexOf("/");
  if(i == -1) {
    browserSetHome();
    return;
  }
  let dir = browserDirectory.substring(0, i);
  browserBrowse(dir);
}




function browserSelectItem(elem) {
  if(browserSelectedItem)
    browserSelectedItem.classList.remove("selected");
  elem.classList.add("selected");
  browserSelectedItem = elem;
  let p = elem.getElementsByTagName("p")[0].innerHTML;
  browserFormInput.value = browserDirectory + "/" + p;
}

function browserSelectDirectory(elem) {
  if(browserSelectedItem)
    browserSelectedItem.classList.remove("selected");
  elem.classList.add("selected");
  browserSelectedItem = elem;
  browserFormInput.value = "";
}

function browserOpenDirectory(elem) {
  let p = elem.getElementsByTagName("p")[0].innerHTML;
  if(browserDirectory == "")
    browserBrowse(p);
  else
    browserBrowse(browserDirectory + "/" + p);
}




window.addEventListener(
    "load",
    function() {
      browserWindow = document.getElementById("media-loader-browser-win");
      browserDirectoryLabel = document.getElementById("browser-navi-directory");
      browserPanel = document.getElementById("browser-panel");
      browserFormInput = browserWindow.getElementsByTagName("input")[0];
      let open = document.getElementById("media-loader-buttons").children[0];
      open.onclick = browserOpen;
      let upHome = document.getElementById("browser-navi-home");
      upHome.onclick = browserSetHome;
      let upNavi = document.getElementById("browser-navi-up");
      upNavi.onclick = browserOnUpNavigate;
    }
  );
