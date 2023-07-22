//
// HepEVD
//

import * as THREE from "three";
import Stats from "three/addons/libs/stats.module.js";

import { RenderState } from "./render_state.js";
import { animate, onWindowResize } from "./rendering.js";
import { saveEvd, quitEvd } from "./ui.js";

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
const mcHits = await fetch("mcHits").then((response) => response.json());

// And use that data to setup the initial rendering states.
const threeDRenderer = new RenderState(
  "3D",
  threeDCamera,
  renderer,
  hits.filter((hit) => hit.type === "3D"),
  mcHits.filter((hit) => hit.type === "3D"),
  detectorGeometry,
);
const twoDRenderer = new RenderState(
  "2D",
  twoDCamera,
  renderer,
  hits.filter((hit) => hit.type === "2D"),
  mcHits.filter((hit) => hit.type === "2D"),
  detectorGeometry,
);
threeDRenderer.otherRenderer = twoDRenderer;
twoDRenderer.otherRenderer = threeDRenderer;
const renderStates = new Map([
  ["3D", threeDRenderer],
  ["2D", twoDRenderer],
]);

// Prefer drawing 3D hits, but draw 2D if only option.
const defaultDraw = threeDRenderer.hitSize != 0 ? "3D" : "2D";

// For each of the 2D + 3D renderers, setup and render the geometry and hits,
// but only show the default one, as picked above.
renderStates.forEach((state) => {
  state.setupUI(defaultDraw);
});

// Hook up various global events.
document.saveEvd = () => saveEvd(renderer);
document.quitEvd = () => quitEvd();
window.addEventListener(
  "resize",
  () => {
    onWindowResize(threeDRenderer.camera, renderer);
    onWindowResize(twoDRenderer.camera, renderer);
  },
  false,
);
document.resetView = () => {
  threeDRenderer.resetView();
  twoDRenderer.resetView();
};

// Finally, animate the scene!
animate(renderer, renderStates, stats);
