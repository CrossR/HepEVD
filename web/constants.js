//
// HepEVD Constant and Configuration
//

import * as THREE from "three";
import { LineMaterial } from "three/addons/lines/LineMaterial.js";

//==============================================================================
// Three.js Constants
//==============================================================================

export const threeDGeoMat = new THREE.LineBasicMaterial({
  color: "darkred",
});
export const threeDTrapezoidMat = new THREE.LineBasicMaterial({
  side: THREE.FrontSide,
  color: "gray",
  transparent: true,
  opacity: 0.1,
  depthWrite: false,
  depthTest: false,
  alphaTest: 0.5,
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

const selectedMaterial2D = (theme) =>
  new THREE.MeshBasicMaterial({
    side: THREE.DoubleSide,
    transparent: true,
    opacity: 0.1,
    color: theme === "dark" ? "yellow" : "darkred",
    depthFunc: THREE.AlwaysDepth,
  });

const selectedMaterial3D = (theme) =>
  new THREE.MeshBasicMaterial({
    side: THREE.DoubleSide,
    transparent: true,
    opacity: 0.05,
    color: theme === "dark" ? "yellow" : "darkred",
    depthFunc: THREE.AlwaysDepth,
  });
export const materialParticle = new THREE.MeshBasicMaterial({
  side: THREE.DoubleSide,
});

//==============================================================================
// UI Constants
//==============================================================================

export const GITHUB_URL = "https://github.com/CrossR/HepEVD";

export const BUTTON_ID = {
  None: "None",
  All: "All",
  Ignored: ["id"],
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
    hitSize: 2,
    materialHit: materialHit,
    selectedMaterial: selectedMaterial2D,
    materialParticle: materialParticle,
  },
  "3D": {
    hitSize: 2,
    materialHit: materialHit,
    selectedMaterial: selectedMaterial3D,
    materialParticle: materialParticle,
  },
};

export const MARKER_CONFIG = {
  point: {
    size: 1.5,
    colour: "red",
  },
  line: {
    size: 0.002,
    colour: "blue",
  },
};

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

//==============================================================================
// Apply GUI Config
//==============================================================================

export function applyConfig(config, renderStates) {
  // Check isn't undefined or empty.
  if (config === undefined) {
    return;
  }

  if (Object.keys(config).length === 0) {
    return;
  }

  if (!config.show2D) {
    renderStates.get("2D").scene.visible = false;
  }
  if (!config.show3D) {
    renderStates.get("3D").scene.visible = false;
  }

  // Only apply the colour updates to raw hits, not particles which should
  // already have an assigned colour...
  if (config.hits.colour !== "") {
    HIT_CONFIG["2D"].materialHit.color.set(config.hits.colour);
    HIT_CONFIG["3D"].materialHit.color.set(config.hits.colour);
  }

  // Whereas the size should apply to both particles and hits...
  if (config.hits.size !== 0.0) {
    HIT_CONFIG["2D"].hitSize = config.hits.size;
    HIT_CONFIG["3D"].hitSize = config.hits.size;
  }

  // As should the opacity...
  if (config.hits.opacity !== 0.0) {
    HIT_CONFIG["2D"].materialHit.opacity = config.hits.opacity;
    HIT_CONFIG["2D"].materialParticle.opacity = config.hits.opacity;
    HIT_CONFIG["3D"].materialHit.opacity = config.hits.opacity;
    HIT_CONFIG["3D"].materialParticle.opacity = config.hits.opacity;
  }

  // TODO: Fix UI buttons if the 2D or 3D scene is hidden.
}
