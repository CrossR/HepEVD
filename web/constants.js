//
// HepEVD Constant and Configuration
//

import * as THREE from "three";
import { LineMaterial } from "three/addons/lines/LineMaterial.js";

//==============================================================================
// UI Constants
//==============================================================================

export const BUTTON_ID = {
  None: "None",
  All: "All",
};

export const THEME = {
  dark: "rgb(25, 30, 36)",
  light: "rgb(242, 242, 242)",
};

export const TO_THEME = {
  dark: THEME["light"],
  light: THEME["dark"],
};

export const DEFAULT_HIT_CLASS = "Hit";

export const DEFAULT_LUT_CONFIG = {
  name: "viridis",
  size: 256,
  maxSize: -1, // INFO: Scale the LUT as needed.
};

export const DEFAULT_CATEGORICAL_LUT_CONFIG = {
  name: "tableau20",
  size: 20,
  maxSize: 20, // INFO: Keep the LUT at a fixed size.
};

export const HIT_CONFIG = {
  "2D": {
    hitSize: 1,
  },
  "3D": {
    hitSize: 0.5,
  },
};

export const MARKER_CONFIG = {
  point: {
    size: 0.5,
  },
};

//==============================================================================
// Three.js Constants
//==============================================================================

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

//==============================================================================
// Physics Constants
//==============================================================================

export const PDG_TO_COLOUR = {
  11: "skyblue", // Elecron : Light Blue
  13: "green", // Muon : Green
  211: "orange", // Pion : Orange
  2212: "red", // Proton : Red
};

export const INTERACTION_TYPE_SCORE = {
  Neutrino: 0,
  Beam: 1,
  Cosmic: 2,
  Other: 3,
};
