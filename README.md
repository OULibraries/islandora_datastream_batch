
## Introduction

This module provides functions to batch download, delete, or replace some or all types of datastream files of all the objects in a collection.

How to use:

download to /tmp/collection_pid:<br>
drush -u 1 --root=/var/www/drupal -l http://localhost islandora_collection_objects_datastream_batch collection_pid 1 MODS

replace from /tmp/collection_pid:<br>
drush -u 1 --root=/var/www/drupal -l http://localhost islandora_collection_objects_datastream_batch collection_pid 2 MODS

Notes:
May need to run with sudo;
Currently the folder to store the datastream files is /tmp, however it will be configurable to users;
