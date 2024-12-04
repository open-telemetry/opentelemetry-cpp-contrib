const express = require('express');
const app = express();
const port = 3500;

const traceparentRegex = /00-[0-9a-f]{32}-[0-9a-f]{16}-0[0-1]/;

app.get('/', (req, res) => {
  let traceparent = req.header("traceparent");
  if (!traceparentRegex.test(traceparent)) {
    throw "Missing traceparent header";
  }

  res.json({traceparent: traceparent});
});

app.get("/b3", (req, res) => {
  let header = req.header("b3");
  if (!/[0-9a-f]{32}-[0-9a-f]{16}-[0-1]{1}/.test(header)) {
    throw "Missing b3 header";
  }

  res.json({"b3": header});
});

app.get("/b3multi", (req, res) => {
  let traceId = req.header("x-b3-traceid");
  let spanId = req.header("x-b3-spanid");
  let sampled = req.header("x-b3-sampled");

  if (!/^([0-9a-f]{32}|[0-9a-f]{16})$/.test(traceId)) {
    throw "Missing x-b3-traceid header";
  }

  if (!/^[0-9a-f]{16}$/.test(spanId)) {
    throw "Missing x-b3-spanid header";
  }

  if (!["0", "1"].includes(sampled)) {
    throw "Missing x-b3-sampled header";
  }

  res.json({
    "x-b3-traceid": traceId,
    "x-b3-spanid": spanId,
    "x-b3-sampled": sampled
  });
});

app.get("/off", (req, res) => {
  if (req.header("traceparent") !== undefined) {
    throw "Found traceparent header, but expected none";
  }

  res.json({});
});

const server = app.listen(port, () => {
  console.log(`simple_express ready at http://localhost:${port}`)
});

process.on("SIGTERM", () => server.close());
process.on("SIGINT", () => server.close());
