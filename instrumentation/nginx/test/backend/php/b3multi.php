<?php
  $b3_trace_id = $_SERVER["HTTP_B3_TRACEID"];
  $b3_span_id = $_SERVER["HTTP_B3_SPANID"];
  $b3_sampled = $_SERVER["HTTP_B3_SAMPLED"];

  if (!preg_match("/^([0-9a-f]{32}|[0-9a-f]{16})$/", $b3_trace_id)) {
    throw new Exception("invalid or missing x-b3-traceid header");
  }

  if (!preg_match("/^[0-9a-f]{16}$/", $b3_span_id)) {
    throw new Exception("invalid or missing x-b3-spanid header");
  }

  if (!preg_match("/^[0-1]$/", $b3_sampled)) {
    throw new Exception("invalid or missing x-b3-sampled header");
  }

  header("Content-Type: application/json");
  echo(json_encode(array(
    "x-b3-traceid" => $b3_trace_id,
    "x-b3-spanid" => $b3_span_id,
    "x-b3-sampled" => $b3_sampled
  )));
?>
