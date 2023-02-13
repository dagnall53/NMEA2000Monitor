const char Header[] PROGMEM = "<!DOCTYPE HTML>\r\n"
  "<html>\r\n"
  "<head>\r\n"
  "<title>N2k MONITOR/Display</title>\r\n"
  "<meta charset=\"UTF-8\">\r\n"      // for special characteres in different languages to appear correctly
  "<meta http-equiv=\"Cache-Control\" content=\"no-cache, no-store, must-revalidate\" />\r\n"
  "<meta http-equiv=\"Pragma\" content=\"no-cache\" />\r\n"
  "<meta http-equiv=\"Expires\" content=\"0\" />\r\n"
  "<link rel=\"icon\" href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQAgMAAABinR\r\n"
  "fyAAAAB3RJTUUH5QocDAwSjDQXNwAAAAlwSFlzAAALEgAACxIB0t1+/AAAAAlQTFRFAAAAADiE////cLcd+QAAADRJ\r\n"
  "REFUCNdjCAUChqxVq1YypIaGRjKkrVo1kyEtNBRIZK0EEqmRmARYAqwEpBisDWwAyCgAkj8dV66qaJcAAAAASUVORK\r\n"
  "5CYII=\" type=\"image/x-png\" />\r\n"
  "<style>\r\n"


  ".div2 {float:left; width:100%; font-size:90%;}\r\n"
  "@media only screen and (orientation: portrait) {.div2 {float:left; width:100%; font-size:60%;}}\r\n"
  ".div3 {float:left; width:50%;}\r\n"
 

  ".div5 {float:left; width:25%;}\r\n"
  ".div6 {float:left; width:14%;}\r\n"
  ".div6x {float:left; width:15%;}\r\n"  
   ".div7 {float:left; width:55%;}\r\n"

  ".inp {border: 1px solid #0F458D; width:55%;font-size:100%; text-align:center;}\r\n"

  ".sel {border: 1px solid #0F458D; width:53%;font-size:100% ;text-align:center;}\r\n" 
  ".rad {float:right; margin-left:25px; height:1.5em; width:1.5em;}\r\n"
  ".but {height:30px; font-size:15px; width:30%; background:#0F458D; color:#fff; border-radius:6px; cursor:pointer;}\r\n"
  ".fld {margin:15px; border-color:#0F458D; padding:1em; border-radius:10px;}\r\n"
  "p.lucida { font-family: \"Lucida Console\", Times, serif;}\r\n"
  "</style>\r\n"
  "</head>\r\n"
  "<body style=\"color:rgb(16,69,141); font-family:verdana,arial\">\r\n"
  "<div class=\"div8\">\r\n"
  "<div class=\"div5\">&nbsp;</div>\r\n"
  "<div class=\"div3\">TITLE </div>\r\n"
  "<div class=\"div5\">\r\n";


// the following is only used in the main settings webpage!

const char OTA_STYLE[] PROGMEM = "<style>#file-input,input{width:100%;height:30px;border-radius:6px;margin:10px auto;font-size:15px}\r\n"
"#file-input{padding:0;border:1px solid #0F458D;line-height:30px;text-align:center;display:block;cursor:pointer}\r\n"
"#bar{background-color:#0F458D;width:0%;border-radius:3px;height:15px}\r\n"
"form{max-width:258px;margin:5px auto;padding:30px;border-radius:6px;text-align:center}\r\n"
".btn{background:#0F458D;color:#fff;cursor:pointer}</style>";

const char OTA_UPLOAD[] PROGMEM = R"=+=+(
<form method="POST" action="ota" enctype="multipart/form-data" id="upload_form">
   <input type="file" name="update" id="file" onchange="sub(this)" style=display:none>
   <label id="file-input" for="file">CHOOSE FILE ...</label>
   <input type="submit" class=btn value="UPLOAD"> 
   <a href="/."><input type="button" class=btn value="HOME"></input></a> 
   <div id="prg"></div>
   <div id="bar"></div>
</form>
<script>
function sub(obj){
  var fileName = obj.value.split("\\\\");
  document.getElementById("file-input").innerHTML = " " + fileName[fileName.length-1];
};
var domReady = function(callback) {
  document.readyState === "interactive" || document.readyState === "complete" ? callback() : document.addEventListener("DOMContentLoaded", callback);
};
domReady(function() {
  var myform = document.getElementById('upload_form');
  var filez  = document.getElementById('file');
  myform.onsubmit = function(event) {
    event.preventDefault();
    var formData = new FormData();
    var file     = filez.files[0];
    if (!file) { return false; }
    formData.append("files", file, file.name);
    var xhr = new XMLHttpRequest();
    xhr.upload.addEventListener("progress", function(evt) {
      if (evt.lengthComputable) {
        var per = Math.round((evt.loaded / evt.total) * 100);
        var prg = document.getElementById('prg');
        prg.innerHTML     ="PROGRESS:" + per + "%"
        var bar = document.getElementById('bar');
        bar.style.width   = per + "%"
      }
    }, false);
    xhr.open('POST', location.href, true);
    // Set up a handler for when the request finishes.
    xhr.onload = function () {
      if (xhr.status === 200) {
        alert('Success!');
      } else {
        alert('An error occurred!');
      }
    };
    xhr.send(formData);
   }
});
</script>)=+=+";


