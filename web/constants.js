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
  style: "continuous",
  name: "cooltowarm",
  size: 128,
};

export const DEFAULT_CATEGORICAL_LUT_CONFIG = {
  style: "categorical",
  name: "tableau20",
  size: 20,
};

export const HIT_CONFIG = {
  "2D": {
    hitSize: 1,
  },
  "3D": {
    hitSize: 1,
  },
};

export const MARKER_CONFIG = {
  point: {
    size: 1,
    colour: "red",
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
  11: "skyblue", // e-
  13: "palegreen", // mu-
  22: "yellow", // Photon
  211: "coral", // Pi+
  2212: "crimson", // Proton

  // Vaguely inverse of the above
  "-11": "darkblue", // e+
  "-13": "darkgreen", // mu+
  "-211": "darkorange", // Pi-
  "-2212": "darkred", // Anti proton
};

export const INTERACTION_TYPE_SCORE = {
  Neutrino: 0,
  Beam: 1,
  Cosmic: 2,
  Other: 3,
};
