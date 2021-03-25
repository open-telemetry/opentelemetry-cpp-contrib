<?php
  $traceparent = $_SERVER["HTTP_TRACEPARENT"];
  if (!preg_match("/00-[0-9a-f]{32}-[0-9a-f]{16}-0[0-1]/", $traceparent)) {
    echo("invalid or missing traceparent");
    throw new Exception("invalid or missing traceparent");
  }

  header("Content-Type: application/json");
  echo(json_encode(array("traceparent" => $traceparent)));
?>
