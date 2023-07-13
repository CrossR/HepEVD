//
// HepEVD Constant and Configuration
//

import * as THREE from "three";

// Mock enum for the default button classes.
export const BUTTON_ID = {
  None: "None",
  All: "All",
};

export const HIT_CONFIG = {
  "2D": {
    "hitSize" : 2
  },
  "3D": {
    "hitSize" : 3
  },
};

export const materialLine = new THREE.LineBasicMaterial({ color: "gray" });
export const materialGeometry = new THREE.LineBasicMaterial({
  color: "darkred",
});
export const materialHit = new THREE.MeshBasicMaterial({
  side: THREE.DoubleSide,
});
