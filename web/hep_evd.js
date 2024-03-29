//
// HepEVD
//

import * as THREE from "three";
import Stats from "three/addons/libs/stats.module.js";

import { THEME } from "./constants.js";
import { getData } from "./data_loader.js";
import { RenderState } from "./render_state.js";
import { animate, onWindowResize } from "./rendering.js";
import { nextState, previousState, updateStateUI } from "./states.js";
import {
  fixThemeButton,
  loadState,
  pickColourscheme,
  quitEvd,
  saveState,
  screenshotEvd,
  toggleTheme,
} from "./ui.js";
import { highlightParticleOnMouseMove } from "./interactions.js";

// Set off the data loading straight away.
// For big events, this can take a while, so we want to do it in parallel with
// the rest of the setup.
const data = getData();

// Do some initial threejs setup...
const threeDCamera = new THREE.PerspectiveCamera(
  50,
  window.innerWidth / window.innerHeight,
  0.1,
  1e6
);
const twoDCamera = new THREE.OrthographicCamera(
  window.innerWidth / -2,
  window.innerWidth / 2,
  window.innerHeight / 2,
  window.innerHeight / -2,
  -1,
  1e6
);
const renderer = new THREE.WebGLRenderer({
  alpha: true,
  antialias: true,
  preserveDrawingBuffer: true,
});

// Add FPS counter for debugging.
const stats = new Stats();
stats.domElement.style.cssText = "position:absolute; bottom:0px; right:0px;";
renderer.setSize(window.innerWidth, window.innerHeight);

const themeName = localStorage.getItem("theme") ?? "dark";
renderer.setClearColor(THEME[themeName]);

document.body.appendChild(renderer.domElement);
document.body.appendChild(stats.dom);

// Now we need to wait for the data to load...
const { hits, mcHits, markers, particles, detectorGeometry } = await data;

// And use that data to setup the initial rendering states.
const threeDRenderer = new RenderState(
  "3D",
  threeDCamera,
  renderer,
  particles,
  hits.filter((hit) => hit.position.dim === "3D"),
  mcHits.filter((hit) => hit.position.dim === "3D"),
  markers.filter((marker) => marker.position.dim === "3D"),
  detectorGeometry
);
const twoDRenderer = new RenderState(
  "2D",
  twoDCamera,
  renderer,
  particles,
  hits.filter((hit) => hit.position.dim === "2D"),
  mcHits.filter((hit) => hit.position.dim === "2D"),
  markers.filter((marker) => marker.position.dim === "2D"),
  detectorGeometry
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

// Finally, animate the scene.
animate(renderer, renderStates, stats);

// Now that we've animated once, hook up event listeners for any change.
renderStates.forEach((state) => {
  state.addEventListener("fullUpdate", () => {
    state.renderEvent(true);
    animate(renderer, renderStates, stats);
  });
  state.addEventListener("change", () => {
    animate(renderer, renderStates, stats);
  });
  state.controls.addEventListener("change", () => {
    animate(renderer, renderStates, stats);
  });
});

// Final tidy ups.
// Hook up various global events and tidy functions.
document.screenshotEvd = () => screenshotEvd(renderer);
document.quitEvd = () => quitEvd();
document.toggleTheme = () => toggleTheme(renderStates);
document.saveState = () => saveState(renderStates);
document.loadState = () => loadState(renderStates);
document.pickColourscheme = () => pickColourscheme(renderStates);
document.nextState = () => nextState(renderStates);
document.prevState = () => previousState(renderStates);
window.addEventListener(
  "resize",
  () => {
    onWindowResize(threeDRenderer, renderer);
    onWindowResize(twoDRenderer, renderer);
  },
  false
);
document.resetView = () => {
  threeDRenderer.resetView();
  twoDRenderer.resetView();
};
fixThemeButton();
updateStateUI(renderStates);

// Add in interactions...
let currentlyHighlighting = [];
const canvas = renderer.domElement;
canvas.addEventListener("mousemove", (event) => {
  currentlyHighlighting = highlightParticleOnMouseMove(
    renderStates,
    currentlyHighlighting,
    event
  );
});

// Test out filter, run every 0.5 seconds
const currentFilters = [];
let filterActive = false;
let previousFilter = "";
const filterTest = document.getElementById("filter_test");

filterTest.addEventListener("input", () => {

  if (filterTest.value === "")
    return;

  const views = ["U View", "V View", "W View"];
  const filter = filterTest.value;

  if (filter === previousFilter)
    return;

  previousFilter = filter;

  if (!views.includes(filter) && !filterActive) {
    console.log(`Invalid filter: ${filter}`);
    return;
  } else if (!views.includes(filter) && filterActive) {
    // Filter was active, but now we're resetting.
    console.log(`Resetting filter`);
    renderStates.forEach((state) => {
      if (!state.visible) return;
      currentFilters.forEach((filter) => {
        state.hitTypeState.removeHitType(filter);
      });
      state.triggerEvent("fullUpdate");
    });

    filterActive = false;
    currentFilters.length = 0;

    return;
  }

  console.log(`Filtering for ${filter}`);

  renderStates.forEach((state) => {
    if (!state.visible) return;
    state.hitTypeState.addHitType(filter);
    state.triggerEvent("fullUpdate");
  });

  currentFilters.push(filter);
  filterActive = true;
});
