var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

Pebble.addEventListener('showConfiguration', function() {
  console.log('Showing configuration page');
  Pebble.openURL(clay.generateUrl());
});