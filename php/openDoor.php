<?php

$cardId = htmlspecialchars($_GET["cardId"]);

if (!isset($cardId)) die("ERROR: CardId not present.");

echo "AccesGranted";

// pokracovani v kolonialnim repu