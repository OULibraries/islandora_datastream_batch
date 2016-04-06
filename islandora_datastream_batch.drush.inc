<?php

/* 
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

define('ISLANDORA_COLLECTION_OBJECTS_DATASTREAM_BATCH_PATH', '/tmp');
define('ISLANDORA_COLLECTION_OBJECTS_DATASTREAM_BATCH_DATASTREAM_TYPES', 'RELS-EXT,MODS,DC,TN,PDF');

function islandora_datastream_batch_drush_command() {
  $items = array();
  $items['islandora_collection_objects_datastream_batch'] = array(
    'callback' => 'islandora_collection_objects_datastream_batch_drush',
    'description' => 'generates islandora PDF books',
    'file' => 'includes/datastream_batch.inc',
    'arguments' => array(
        'pid' => "The Fedora PID to process",
        'opera' => "The operation type of the drush command",
        'datastream_types' => 'The datastream types to process',
    ),
    'options' => array(),
    'examples' => array(
      'simple example' => 'drush -u 1 --root=/var/www/drupal -l http://localhost islandora_collection_objects_datastream_batch islandora:1 1 MODS',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );

  return $items;
}

function islandora_collection_objects_datastream_batch_drush($pid, $opera, $datastream_types = null) {
    module_load_include('module', 'islandora_basic_collection', 'islandora_basic_collection');
    $object = islandora_object_load($pid);
    if(null != $object){
        $collection_path = ISLANDORA_COLLECTION_OBJECTS_DATASTREAM_BATCH_PATH . "/" . $pid;
        if(!file_exists($collection_path)){
            mkdir($collection_path);
        }
        
        $datastream_types = strtoupper($datastream_types);
        $datastream_types = str_replace(" ", "", $datastream_types);
        if($datastream_types == "ALL"){
            $datastream_types = explode(",", ISLANDORA_COLLECTION_OBJECTS_DATASTREAM_BATCH_DATASTREAM_TYPES);
        }
        else{
            $datastream_types = explode(",", $datastream_types);
        }
        
        $member_objects = islandora_basic_collection_get_member_objects($object, 0, -1, $type = 'view', $cmodel = NULL);
        if(null != $member_objects && is_array($member_objects) && !empty($member_objects[1])){
            foreach($member_objects[1] as $member_object_info){
                $member_pid = $member_object_info['object']['value'];
                if(!empty($member_pid)){
                    $member_object_path = $collection_path . "/" . $member_pid;
                    if(!file_exists($member_object_path)){
                        mkdir($member_object_path);
                    }
                    process_datastream_operations($member_pid, $datastream_types, $opera, $member_object_path);
                }
                else{
                    continue;
                }
            }
        }
        else{
            drupal_set_message(t("This collection object has NO member objects!"), 'warning');
        }
    }
    else{
        drupal_set_message(t("This collection object does NOT exist!"), 'error');
    }
}

/**
 * 
 * @param type $member_pid : object PID
 * @param type $datastream_types : RELS-EXT,MODS,DC,TN,PDF, or ALL for all types; 
 *                                 Multiple types are separated by comma: e.g. can type in 'MODS,TN' for MODS and TN types
 * @param type $opera : 1 => download; 2 => delete; 3 => replace
 * @param type $member_object_path : the path to store the downloaded datastream files
 * 
 * 
 */
function process_datastream_operations($member_pid, $datastream_types, $opera, $member_object_path){
    foreach ($datastream_types as $type){
        $datastream_object = islandora_datastream_load($type, $member_pid);
        //replace_datastream($datastream_object, $member_pid, $member_object_path);
        download_datastream($datastream_object, $member_object_path, NULL);
    }
}

/**
 * Callback function to view or download a datastream.
 *
 * @note
 *  This function calls exit().
 *
 * @param AbstractDatastream $datastream
 *   The datastream to view/download.
 * @param bool $download
 *   If TRUE the file is download to the user computer for viewing otherwise it
 *   will attempt to display in the browser natively.
 * @param int $version
 *   The version of the datastream to display
 */
function download_datastream(AbstractDatastream $datastream, $filepath, $version = NULL) {
    module_load_include('inc', 'islandora', 'includes/mimetype.utils');
    module_load_include('inc', 'islandora', 'includes/datastream');
    // XXX: Certain features of the Devel module rely on the use of "shutdown
    // handlers", such as query logging... The problem is that they might blindly
    // add additional output which will break things if what is actually being
    // output is anything but a webpage... like an image or video or audio or
    // whatever the datastream is here.
    $GLOBALS['devel_shutdown'] = FALSE;

    if ($version !== NULL) {
      if (isset($datastream[$version])) {
        $datastream = $datastream[$version];
      }
      else {
        return drupal_not_found();
      }
    }
    header('Content-type: ' . $datastream->mimetype);
    if ($datastream->controlGroup == 'M' || $datastream->controlGroup == 'X') {
      header('Content-length: ' . $datastream->size);
    }

    $filename = get_datastream_filename($datastream);
    header("Content-Disposition: attachment; filename=\"$filename\"");

    $cache_check = islandora_view_datastream_cache_check($datastream);
    if ($cache_check !== 200) {
      if ($cache_check === 304) {
        header('HTTP/1.1 304 Not Modified');
      }
      elseif ($cache_check === 412) {
        header('HTTP/1.0 412 Precondition Failed');
      }
    }
    islandora_view_datastream_set_cache_headers($datastream);

    drupal_page_is_cacheable(FALSE);

    // New content needed.
    if ($cache_check === 200) {
      $chunk_headers = FALSE;
      if (isset($_SERVER['HTTP_RANGE'])) {
        // Set headers specific to chunking.
        $chunk_headers = islandora_view_datastream_set_chunk_headers($datastream);
      }
      // Try not to load the file into PHP memory!
      // Close and flush ALL the output buffers!
      while (@ob_end_flush()) {
      };

      if (isset($_SERVER['HTTP_RANGE'])) {
        if ($chunk_headers) {
          islandora_view_datastream_deliver_chunks($datastream, $chunk_headers);
        }
      }
      else {
        $filename = $filepath.'/'.$filename;
        $datastream->getContent($filename);
      }
    }
}

function replace_datastream(AbstractDatastream $ds, $object_id, $file_path){
    
    $object = islandora_object_load($object_id);
    $form_state['redirect'] = "islandora/object/{$object->id}";
    $file = $file_path . "/" . get_datastream_filename($ds);
    
    try {
      $ds->setContentFromFile($file);
    }
    catch (exception $e) {
      drupal_set_message(t('An error occurred during datastream updates. See watchlog for more information.'), 'error');
      watchdog('islandora',
        'Failed to add new versionable datastream.<br/>code: @code<br/>message: @msg',
        array(
          '@code' => $e->getCode(),
          '@msg' => $e->getMessage(),
        ),
        WATCHDOG_ERROR
      );
      file_delete($file);
      return;
    }
    drupal_set_message(t("Successfully Updated Datastream"));
}

function get_datastream_filename(AbstractDatastream $datastream){
    
    module_load_include('inc', 'islandora', 'includes/datastream');
    module_load_include('inc', 'islandora', 'includes/mimetype.utils');
    
    $extension = '.' . islandora_get_extension_for_mimetype($datastream->mimetype);
    $label = $datastream->label;
    $extension_length = strlen($extension);
    $duplicate_extension_position = strlen($label) > $extension_length ?
      strripos($label, $extension, -$extension_length) :
      FALSE;
    $filename = $label;
    if ($duplicate_extension_position === FALSE) {
      $filename .= $extension;
    }
    return $filename;
}