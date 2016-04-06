<?php

/* 
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

function get_collection_objects() {
    /**
    * Searches through available collection objects.
    *FILTER(regex(?label, '$search_value', 'i') || regex(str(?pid), '$search_value', 'i'))
    * @param string $search_value
    *   Returns a json array of matching collections.
    */
    $tuque = islandora_get_tuque_connection();
    $sparql_query = <<<EOQ
        SELECT ?pid ?label
        WHERE {
          ?pid <fedora-model:label> ?label ;
               <fedora-model:hasModel> <info:fedora/islandora:collectionCModel> .
          
        }
EOQ;
    $results = $tuque->repository->ri->sparqlQuery($sparql_query);
    $return = array();
    foreach ($results as $objects) {
      $return[$objects['pid']['value']] = t('@label (@pid)', array(
        '@label' => $objects['label']['value'],
        '@pid' => $objects['pid']['value'],
      ));
    }
    //drupal_json_output($return);
    return $return;
}