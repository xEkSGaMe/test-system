const express = require("express");
const app = express();
const port = 3000;

app.get("/", (req, res) => {
  res.send("Web Client - Development Mode");
});

app.listen(port, () => {
  console.log(`Web client running on port ${port}`);
});