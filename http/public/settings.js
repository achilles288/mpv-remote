var settingsSelectedMenu = null;




function settingsMenuSelect(event) {
  settingsMenuClose(event);
  this.setAttribute("selected", "1");
  this.onclick = null;
  let slide = this.getElementsByClassName("slide")[0];
  slide.style.display = "block";
  settingsSelectedMenu = this;
}

function settingsMenuClose(event) {
  if(settingsSelectedMenu != null) {
    settingsSelectedMenu.setAttribute("selected", "0");
    let slide = settingsSelectedMenu.getElementsByClassName("slide")[0];
    slide.style.display = "none";
    var temp = settingsSelectedMenu;
    settingsSelectedMenu = null;
    setTimeout(
      (event) => temp.onclick = settingsMenuSelect,
      100
    );
  }
}




function settingsOnChangePassword(event) {
  event.preventDefault();
  let formData = new FormData(event.target);
  let pswd1 = formData.get("password");
  let pswd2 = formData.get("confirm");
  if(pswd1 != pswd2) {
    event.target.getElementsByClassName("error")[0].innerHTML
      = "Please reconfirm the password";
    return;
  }
  formData.delete("confirm");
  
  fetch(
    "change-password",
    {
      method: "POST",
      body: formData,
      credentials: "same-origin"
    }
  )
    .then(response => {
      if(response.status == 200) {
        event.target.getElementsByClassName("error")[0].innerHTML = "";
        alert("Password changed");
      }
      else if(response.status == 401) {
        event.target.getElementsByClassName("error")[0].innerHTML
          = "Incorrect password";
      }
      else {
        event.target.getElementsByClassName("error")[0].innerHTML = "Error";
      }
    })
    .catch((error) => console.error(error));
}




window.addEventListener(
  "load",
  function() {
    let list = document.getElementById("settings-list").children;
    for(let i=0; i<list.length; i++) {
      list[i].onclick = settingsMenuSelect;
      list[i].getElementsByClassName("close")[0].onclick = settingsMenuClose;
    }
    document.getElementById("settings-change-pswd").onsubmit
      = settingsOnChangePassword;
  }
);