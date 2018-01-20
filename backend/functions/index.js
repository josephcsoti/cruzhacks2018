const functions = require('firebase-functions');

// The Firebase Admin SDK to access the Firebase Realtime Database. 
const admin = require('firebase-admin');
admin.initializeApp(functions.config().firebase);

const database = admin.database();

// Create Lock Request
exports.LockRequestQueue = functions.database.ref('/request_queue/lock/{requestID}').onCreate(event => {
  
  const requestID = event.params.requestID;
  const userID = event.auth.variable ? event.auth.variable.uid : null;

  const data = event.data.val();
  const rackID = data.rackID;
  const slot = data.slot + 0;
  const password = data.password;

  console.log("LOCK REQ: ", requestID, rackID, slot, password)

  const p1 = database.ref('/bikerack_secure/' + rackID + '/' + slot).set(password)
              .then(console.log("SECURED"));

  const p2 = database.ref('/lease/' + rackID + '/' + slot).set(userID)
              .then(console.log("LEASED"));
  
  return Promise.all([p1, p2]);
});

// Create Request
exports.UnlockRequestQueue = functions.database.ref('/request_queue/unlock/{requestID}').onCreate(event => {
  
  const requestID = event.params.requestID;
  const userID = event.auth.variable ? event.auth.variable.uid : null;

  const data = event.data.val();
  const rackID = data.rackID;
  const slot = data.slot + 0;
  const password = data.password;

  const p1 = this.database.ref('/lobby_admin/'+lobbyID).once('value').val().toString()

  console.log("UNLOCK REQ: ", requestID, rackID, slot, password)

  const p1 = database.ref('/bikerack_secure/' + rackID + '/' + slot).remove()
                  .then(console.log("SECURED REMOVED"));
    
  const p2 = database.ref('/lease/' + rackID + '/' + slot).remove()
                  .then(console.log("LEASED VOID"));
      
  return Promise.all()[p1, p2];
  
  });

// Realtime Database under the path /messages/:pushId/original
exports.BikerackActivity = functions.https.onRequest((req, res) => {
  // Grab the text parameter.

  const rackID = req.query.rackID;
  const slot = req.query.slot;
  const details = req.query.details;  

  // Push the new message into the Realtime Database using the Firebase Admin SDK.
  admin.database().ref('/bikerack_activity').push({
    slot: slot,
    rackID: rackID,
    details: details
  }).then(
    res.status(200).send(
      "SUCCESS <br>" +
      slot + " " + rackID + " " + details + " " + Date.now()
    )
  );

});

exports.helloWorld = functions.https.onRequest((request, response) => {
  response.send("Hello from CruzHacks 2018!");
});
