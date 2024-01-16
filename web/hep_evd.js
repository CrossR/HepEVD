//
// HepEVD
//

import * as THREE from "three";
import Stats from "three/addons/libs/stats.module.js";

import { HIT_CONFIG, THEME } from "./constants.js";
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
  setTheme,
} from "./ui.js";
import { drawParticleOverlay } from "./hits.js";

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
// Move to top right.
stats.domElement.style.cssText = "position:absolute; bottom:0px; right:0px;";
renderer.setSize(window.innerWidth, window.innerHeight);

const themeName = localStorage.getItem("theme") ?? "dark";
renderer.setClearColor(THEME[themeName]);

document.body.appendChild(renderer.domElement);
document.body.appendChild(stats.dom);

// Pull in the basic data from the API...
const data = await getData();
const { hits, mcHits, markers, particles, detectorGeometry } = data;

// And use that data to setup the initial rendering states.
const threeDRenderer = new RenderState(
  "3D",
  threeDCamera,
  renderer,
  particles,
  hits.filter((hit) => hit.position.dim === "3D"),
  mcHits.filter((hit) => hit.position.dim === "3D"),
  markers.filter((marker) => marker.position.dim === "3D"),
  detectorGeometry,
);
const twoDRenderer = new RenderState(
  "2D",
  twoDCamera,
  renderer,
  particles,
  hits.filter((hit) => hit.position.dim === "2D"),
  mcHits.filter((hit) => hit.position.dim === "2D"),
  markers.filter((marker) => marker.position.dim === "2D"),
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
document.setTheme = () => setTheme(renderStates);
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
  false,
);
document.resetView = () => {
  threeDRenderer.resetView();
  twoDRenderer.resetView();
};
fixThemeButton(true);
updateStateUI(renderStates);

const canvas = renderer.domElement;
canvas.addEventListener("mousemove", (event) => {

  const mouse = new THREE.Vector2();
  mouse.x = (event.clientX / window.innerWidth) * 2 - 1;
  mouse.y = -(event.clientY / window.innerHeight) * 2 + 1;

  let highlightingParticle = false;

  renderStates.forEach((state) => {
    if (!state.visible) {
      return;
    }

    const raycaster = new THREE.Raycaster();
    raycaster.setFromCamera(mouse, state.camera);
    const intersects = raycaster.intersectObjects(state.scene.children, true);
    if (intersects.length > 0) {
      const intersectObject = intersects[0];

      if (intersectObject.object.type !== "Mesh") {
        return;
      }

      const hitId = intersectObject.instanceId;
      const activeHit = state.hitData.activeHits[hitId];
      let activeParticle;

      try {
        const activeParticleId = state.particleData.hitToParticleMap.get(
          activeHit.id,
        );
        activeParticle = state.particleData.particleMap.get(activeParticleId);
      } catch {
        return;
      }

      const parentParticle =
        state.particleData.childToParentMap.get(activeParticle);

      // Finally, lets render out all the hits of this particle, but with a unique glow.
      drawParticleOverlay(
        state.hitGroup,
        state.particleData,
        state.hitData,
        state.hitTypeState,
        HIT_CONFIG[state.hitDim],
        activeParticle,
      );
      state.triggerEvent("change");
      highlightingParticle = true;
    }
  });

  if (! highlightingParticle) {
    renderStates.forEach((state) => {
      if (!state.visible) {
        return;
      }
      state.triggerEvent("fullUpdate");
    });
  }
});
