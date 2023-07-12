//
// HepEVD
//

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import Stats from "three/addons/libs/stats.module.js";

import { DefaultButtonID, materialGeometry, materialHit, materialLine } from "./constants.js";
import { getHitProperties } from "./helpers.js";
import { drawHits, setupControls, setupThreeDControls, setupTwoDControls } from "./hits.js";
import { animate, drawBoxVolume, fitSceneInCamera } from "./rendering.js";
import { hitsToggle, populateDropdown, toggleButton } from "./ui.js";

// First, do the initial threejs setup.
// That is the scene/camera/renderer/controls, and some basic properties of each.
// Once complete, add to the actual web page.
const scenes = new Map();
scenes.set("3D", new THREE.Scene());
scenes.set("2D", new THREE.Scene());

const camera = new THREE.PerspectiveCamera(
  50,
  window.innerWidth / window.innerHeight,
  0.1,
  1e6,
);
const renderer = new THREE.WebGLRenderer({ alpha: true, antialias: true });
const controls = new OrbitControls(camera, renderer.domElement);
const stats = new Stats();
renderer.setSize(window.innerWidth, window.innerHeight);

if (document.body.className === "lighttheme") renderer.setClearColor("white");
else renderer.setClearColor("black");

document.body.appendChild(renderer.domElement);
document.body.appendChild(stats.dom);

// Then, setup some basic data structures the rest of the renderer can use.
const detectorGeometryGroup = new THREE.Group();
scenes.get("3D").add(detectorGeometryGroup);

const hitGroupMap = new Map();
hitGroupMap.set("3D", new Map());
hitGroupMap.set("2D", new Map());

// Default hit groups for the "All" case.
const threeDHitGroup = new THREE.Group();
scenes.get("3D").add(threeDHitGroup);
hitGroupMap.get("3D").set(DefaultButtonID.All, threeDHitGroup);

const twoDHitGroup = new THREE.Group();
scenes.get("3D").add(twoDHitGroup);
hitGroupMap.get("2D").set(DefaultButtonID.All, twoDHitGroup);

// Finally, start pulling in data about the event.
const detectorGeometry = await fetch("geometry").then((response) =>
  response.json(),
);
const hits = await fetch("hits").then((response) => response.json());

// Populate various maps we need for fast lookups.
// First, just a basic one to store all the 3D/2D hits.
const hitMap = new Map([
  ["3D", hits.filter((hit) => hit.type === "3D")],
  ["2D", hits.filter((hit) => hit.type.includes("2D"))],
]);
// Next, one stores the hits being actively rendered.
const activeHitMap = new Map([
  ["3D", new Map()],
  ["2D", new Map()],
]);
// Finally, store a mapping between each hit and the properties of that hit.
const hitPropMaps = getHitProperties(hits);

// Time to start the actual rendering.
detectorGeometry
  .filter((volume) => volume.type === "box")
  .forEach((box) =>
    drawBoxVolume(detectorGeometryGroup, materialGeometry, box),
  );

// Prefer drawing 3D hits, but draw 2D if only option.
const defaultDraw = hitMap.get("3D").length != 0 ? "3D" : "2D";
activeHitMap.get(defaultDraw).set(DefaultButtonID.All, hitMap.get(defaultDraw));
drawHits(
  hitGroupMap.get(defaultDraw).get(DefaultButtonID.All),
  materialHit,
  activeHitMap.get(defaultDraw),
  hitPropMaps.get(defaultDraw),
);

// Populate the UI properly.
// This includes functions that the GUI uses, and filling in the various dropdowns.

// First, setup all the button on click events.
let toggleHits = (hitType) => (toggleTarget) => {
  let newScene = hitsToggle(
    hitMap.get(hitType),
    activeHitMap.get(hitType),
    hitGroupMap.get(hitType),
    hitPropMaps.get(hitType),
    toggleTarget,
  );
  if (newScene) scenes.get(hitType).add(newScene);
  toggleButton(hitType, toggleTarget);
};

// Populate all the dropdowns.
populateDropdown("3D", hitPropMaps.get("3D"), toggleHits("3D"));
populateDropdown("2D", hitPropMaps.get("2D"), toggleHits("2D"));

// Toggle on the default rendering.
toggleButton(defaultDraw, DefaultButtonID.All);

// Start the final rendering of the event.
// Orient the camera to the middle of the scene.
if (defaultDraw == "3D") scenes.get("2D").visible = false;
if (defaultDraw == "2D") scenes.get("3D").visible = false;
fitSceneInCamera(camera, controls, detectorGeometryGroup);
setupControls(defaultDraw, controls);

// Finally, animate the scene!
animate(renderer, scenes, camera, stats);

