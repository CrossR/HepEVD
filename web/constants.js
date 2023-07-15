//
// HepEVD Constant and Configuration
//

import * as THREE from "three";
import { LineMaterial } from "three/addons/lines/LineMaterial.js";

// Mock enum for the default button classes.
export const BUTTON_ID = {
  None: "None",
  All: "All",
};

export const HIT_CONFIG = {
  "2D": {
    hitSize: 3,
  },
  "3D": {
    hitSize: 3,
  },
};

export const threeDGeoMat = new THREE.LineBasicMaterial({
  color: "darkred",
});
export const twoDXMat = new LineMaterial({
  color: "darkred",
  linewidth: 0.002,
});
export const twoDYMat = new LineMaterial({
  color: "darkgreen",
  linewidth: 0.002,
});
export const materialHit = new THREE.MeshBasicMaterial({
  side: THREE.DoubleSide,
});
