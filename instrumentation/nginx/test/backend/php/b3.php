<?php
  $b3 = $_SERVER["HTTP_B3"];
  if (!preg_match("/[0-9a-f]{32}-[0-9a-f]{16}-[0-1]{1}/", $b3)) {
    echo("invalid or missing b3 header");
    throw new Exception("invalid or missing b3 header");
  }

  header("Content-Type: application/json");
  echo(json_encode(array("b3" => $b3)));
?>
