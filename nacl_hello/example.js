// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This function is called by common.js when the NaCl module is
// loaded.
var img;

function moduleDidLoad() {
  // Once we load, hide the plugin. In this example, we don't display anything
  // in the plugin, so it is fine to hide it.
  //common.hideModule();
  
  // After the NaCl module has loaded, common.naclModule is a reference to the
  // NaCl module's <embed> element.
  //
  // postMessage sends a message to it.
  // common.naclModule.postMessage('hello');

  // Add image to canvas
  img = new Image();
  img.src = "rhino.png";
  var canvas = document.getElementById("canvas");
  var ctx = canvas.getContext("2d");
  img.onload = function() {
    ctx.drawImage(img, 0, 0);
  };
}

function sendImageData() {
  var canvas = document.getElementById("canvas");
  var ctx = canvas.getContext("2d");
  
  var imageData = ctx.getImageData(0, 0, img.width, img.height);

  common.naclModule.postMessage({
     'message' : 'texture',
     'name' : "name",
     'width' : img.width,
     'height' : img.height,
     'data' : imageData.data.buffer});

  console.log("Sending image data from browser");
}

// This function is called by common.js when a message is received from the
// NaCl module.
function handleMessage(message) {
  var logEl = document.getElementById('log');
  logEl.textContent += message.data;
}
