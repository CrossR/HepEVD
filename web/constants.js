//
// HepEVD Constant and Configuration
//

import * as THREE from "three";

// Mock enum for the default button classes.
export const DefaultButtonID = {
  None: "None",
  All: "All",
};

export const materialLine = new THREE.LineBasicMaterial({ color: "gray" });
export const materialGeometry = new THREE.LineBasicMaterial({
  color: "darkred",
});
export const materialHit = new THREE.MeshBasicMaterial({
  side: THREE.DoubleSide,
});
