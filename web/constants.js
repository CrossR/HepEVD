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
  opacity: 0.05,
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
export const trackLineMaterial = new LineMaterial({
  color: "darkred",
  linewidth: 2, // in pixels
  resolution: new THREE.Vector2(window.innerWidth, window.innerHeight),
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
  blue: "rgb(41, 22, 131)",
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
    materialParticle: materialParticle,
    renderOrder: 1,
  },
  "3D": {
    hitSize: 2,
    materialHit: materialHit,
    materialParticle: materialParticle,
    renderOrder: 1,
  },
};

export const PARTICLE_CONFIG = {
  menu: {
    showMenu: true,
    maxToShow: 1000,
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
  321: "darkviolet", // K+
  2212: "crimson", // Proton

  // Vaguely inverse of the above
  "-11": "darkblue", // e+
  "-13": "darkgreen", // mu+
  "-211": "darkorange", // Pi-
  "-321": "lightpink", // K-
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

  // If we aren't showing the scene, hide the scenes + button.
  if (!config.show2D) {
    renderStates.get("2D").scene.visible = false;

    const dropDownButton = document.getElementById("2d_dropdown_button");

    if (dropDownButton) dropDownButton.style.display = "none";
  }

  if (!config.show3D) {
    renderStates.get("3D").scene.visible = false;

    const dropDownButton = document.getElementById("3d_dropdown_button");

    if (dropDownButton) dropDownButton.style.display = "none";
  }

  PARTICLE_CONFIG.menu.showMenu = config.showParticleMenu;

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
}
