//
// HepEVD
//

import * as THREE from "three";
import Stats from "three/addons/libs/stats.module.js";

import {
  BUTTON_ID,
  HIT_CONFIG,
  materialHit,
  threeDGeoMat,
} from "./constants.js";
import { getHitProperties, getHitClasses, getDefaultFilters, fixFilters } from "./helpers.js";
import { drawHits, setupControls } from "./hits.js";
import {
  animate,
  drawBoxVolume,
  drawTwoDBoxVolume,
  fitSceneInCamera,
  isSceneActive,
  toggleScene,
} from "./rendering.js";
import {
  hitsToggle,
  isButtonActive,
  populateClassToggle,
  populateDropdown,
  saveEvd,
  toggleButton,
  updateUI,
} from "./ui.js";
import {
  RenderState
} from "./render_state.js";

// Do some initial threejs setup...
const threeDCamera = new THREE.PerspectiveCamera(
  50,
  window.innerWidth / window.innerHeight,
  0.1,
  1e6,
);
const twoDCamera = new THREE.OrthographicCamera(
  window.innerWidth / -2,
  window.innerWidth / 2,
  window.innerHeight / 2,
  window.innerHeight / -2,
  -1,
  1e6,
);
const renderer = new THREE.WebGLRenderer({
  alpha: true,
  antialias: true,
  preserveDrawingBuffer: true,
});
const stats = new Stats();
renderer.setSize(window.innerWidth, window.innerHeight);

if (document.body.className === "lighttheme") renderer.setClearColor("white");
else renderer.setClearColor("black");

document.body.appendChild(renderer.domElement);
document.body.appendChild(stats.dom);

// Pull in the basic data from the API...
const detectorGeometry = await fetch("geometry").then((response) =>
  response.json(),
);
const hits = await fetch("hits").then((response) => response.json());

// And use that data to setup the initial rendering states.
const threeDRenderer = new RenderState(
    "3D", threeDCamera, renderer, hits.filter((hit) => hit.type === "3D"), detectorGeometry
);
const twoDRenderer = new RenderState(
    "2D", twoDCamera, renderer, hits.filter((hit) => hit.type === "2D"), detectorGeometry
);
const renderStates = new Map([
    ["3D", threeDRenderer],
    ["2D", twoDRenderer],
]);

// Prefer drawing 3D hits, but draw 2D if only option.
const defaultDraw = threeDRenderer.hitSize != 0 ? "3D" : "2D";

// const hitClassButtonClick = (hitType) => (hitClass) => {

//   if (isButtonActive("classes", hitClass)) {

//     // Drop the filter for this button and run the fixing.
//     const index = activeHitFilterMap.get(hitType).indexOf(
//         classFilterMap.get(hitType).get(hitClass)
//     );
//     activeHitFilterMap.get(hitType).splice(index, 1);

//     fixFilters(activeHitFilterMap.get(hitType), classFilterMap.get(hitType), false);

//     // Re-enable the general scene, if needed.
//     const currentKey = [...activeHitMap.get(hitType).keys()].sort().join("_");
//     hitGroupMap.get(hitType).get(currentKey).visible = true;

//     toggleButton("classes", hitClass, false);
//     return;
//   }

//   activeHitFilterMap.get(hitType).push(classFilterMap.get(hitType).get(hitClass));
//   fixFilters(activeHitFilterMap.get(hitType), classFilterMap.get(hitType), true);

//   const newScene = new THREE.Group();
//   drawHits(
//     newScene,
//     materialHit,
//     activeHitMap.get(hitType),
//     hitPropMaps.get(hitType),
//     true,
//     HIT_CONFIG[hitType],
//     activeHitFilterMap.get(hitType),
//   );
//   scenes.get(hitType).add(newScene);

//   const currentKey = [...activeHitMap.get(hitType).keys()].sort().join("_");
//   hitGroupMap.get(hitType).get(currentKey).visible = false;

//   toggleButton("classes", hitClass, false);
// }

// Start the final rendering of the event.
// Orient the camera to the middle of the scene.
renderStates.forEach((state) => {
    state.renderGeometry();
    state.renderHits();
    state.setupUI(defaultDraw);
});

// Setup the default UI, with the right buttons and options selected.
toggleButton(defaultDraw, BUTTON_ID.All);
updateUI(defaultDraw, BUTTON_ID.All);
document.saveEvd = () => saveEvd(renderer);

// Finally, animate the scene!
animate(renderer, renderStates, stats);
