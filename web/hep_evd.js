//
// HepEVD
//

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import Stats from "three/addons/libs/stats.module.js";

import { BUTTON_ID, HIT_CONFIG, threeDGeoMat, materialHit } from "./constants.js";
import { getHitProperties } from "./helpers.js";
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
  populateDropdown,
  toggleButton,
} from "./ui.js";

// First, do the initial threejs setup.
// That is the scene/camera/renderer/controls, and some basic properties of each.
// Once complete, add to the actual web page.
const scenes = new Map([
  ["3D", new THREE.Scene()],
  ["2D", new THREE.Scene()],
]);

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
const cameras = new Map([
  ["3D", threeDCamera],
  ["2D", twoDCamera],
]);
const renderer = new THREE.WebGLRenderer({ alpha: true, antialias: true });
const controls = new Map([
  ["3D", new OrbitControls(cameras.get("3D"), renderer.domElement)],
  ["2D", new OrbitControls(cameras.get("2D"), renderer.domElement)],
]);
const stats = new Stats();
renderer.setSize(window.innerWidth, window.innerHeight);

if (document.body.className === "lighttheme") renderer.setClearColor("white");
else renderer.setClearColor("black");

document.body.appendChild(renderer.domElement);
document.body.appendChild(stats.dom);

// Then, setup some basic data structures the rest of the renderer can use.
const detectorGeometryMap = new Map([
  ["3D", new THREE.Group()],
  ["2D", new THREE.Group()],
]);

const hitGroupMap = new Map([
  ["3D", new Map()],
  ["2D", new Map()],
]);

// Finally, start pulling in data about the event.
const detectorGeometry = await fetch("geometry").then((response) =>
  response.json(),
);
const hits = await fetch("hits").then((response) => response.json());

// Populate various maps we need for fast lookups.
// First, just a basic one to store all the 3D/2D hits.
const hitMap = new Map([
  ["3D", hits.filter((hit) => hit.type === "3D")],
  ["2D", hits.filter((hit) => hit.type === "2D")],
]);
// Next, one stores the hits being actively rendered.
const activeHitMap = new Map([
  ["3D", new Map()],
  ["2D", new Map()],
]);
// Finally, store a mapping between each hit and the properties of that hit.
const hitPropMaps = getHitProperties(hits);

// Time to start the actual rendering.

// 3D and 2D geometry drawn individually.
const boxVolumes = detectorGeometry
    .filter((volume) => volume.type === "box");
boxVolumes.forEach((box) =>
    drawBoxVolume(detectorGeometryMap.get("3D"), threeDGeoMat, box),
  );

// Prefer drawing 3D hits, but draw 2D if only option.
const defaultDraw = hitMap.get("3D").length != 0 ? "3D" : "2D";

// Add detector geometry to the relevant scene...
["3D", "2D"].forEach((hitType) => {
  scenes.get(hitType).add(detectorGeometryMap.get(hitType));
});

// Add a default render group to the current target, and render into it.
const defaultHitGroup = new THREE.Group();
scenes.get(defaultDraw).add(defaultHitGroup);
hitGroupMap.get(defaultDraw).set(BUTTON_ID.All, defaultHitGroup);
activeHitMap.get(defaultDraw).set(BUTTON_ID.All, hitMap.get(defaultDraw));

drawHits(
  hitGroupMap.get(defaultDraw).get(BUTTON_ID.All),
  materialHit,
  activeHitMap.get(defaultDraw),
  hitPropMaps.get(defaultDraw),
  false,
  HIT_CONFIG[defaultDraw]
);

// Delay drawing of the 2D geometry, so we can base it on the hits bounding box.
drawTwoDBoxVolume(
    hitMap.get("2D"),
    hitGroupMap.get("2D").get(BUTTON_ID.All),
    detectorGeometryMap.get("2D"),
    cameras.get("2D"),
)

// Populate the UI properly.
// This includes functions that the GUI uses, and filling in the various dropdowns.
// First, setup all the button on click events.
let hitButtonClick = (hitType) => (toggleTarget) => {
  if (
    isButtonActive(hitType, toggleTarget) &&
    !isSceneActive(scenes, hitType)
  ) {
    toggleScene(scenes, controls, hitType);
    return;
  }

  let newScene = hitsToggle(
    hitMap.get(hitType),
    activeHitMap.get(hitType),
    hitGroupMap.get(hitType),
    hitPropMaps.get(hitType),
    HIT_CONFIG[hitType],
    toggleTarget,
  );

  if (newScene) scenes.get(hitType).add(newScene);

  toggleButton(hitType, toggleTarget);
  toggleScene(scenes, controls, hitType);
};

// Populate all the dropdowns.
populateDropdown("3D", hitPropMaps.get("3D"), hitButtonClick("3D"));
populateDropdown("2D", hitPropMaps.get("2D"), hitButtonClick("2D"));

// Toggle on the default rendering.
toggleButton(defaultDraw, BUTTON_ID.All);

// Start the final rendering of the event.
// Orient the camera to the middle of the scene.
toggleScene(scenes, controls, defaultDraw);
["3D", "2D"].forEach((hitType) => {
  fitSceneInCamera(
    cameras.get(hitType),
    controls.get(hitType),
    detectorGeometryMap.get(hitType),
    hitType
  );
  setupControls(hitType, controls.get(hitType));
  scenes.get(hitType).add(cameras.get(hitType));
});

// Finally, animate the scene!
animate(renderer, scenes, cameras, stats);
