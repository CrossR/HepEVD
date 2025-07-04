//
// Interactions
//

import * as THREE from "three";

// Mouseover interactions can be very expensive, so we try to
// limit the number of times we do this by only checking
// every 16 milliseconds (60 FPS).
const INTERACTION_CHECK_INTERVAL = 16;
let mouseTimeout;

// Also store and re-use certain objects to avoid
// creating new ones every time.
const mouse = new THREE.Vector2();
const raycaster = new THREE.Raycaster();

/**
 * Highlights a particle on mouse move, if applicable.
 * I.e. if the mouse is over a particle, dull the others and highlight the one under the mouse.
 *
 * @param {Map<string, RenderState>} renderStates - The render states to check for highlights.
 * @param {Array<string>} currentlyHighlighting - The currently highlighted particles.
 * @param {MouseEvent} event - The mouse event to check for highlights.
 * @returns {Array<string>} - The IDs of the particles that are currently highlighted.
 */
export function highlightParticleOnMouseMove(
  renderStates,
  currentlyHighlighting,
  event,
) {
  if (mouseTimeout) return currentlyHighlighting;

  mouseTimeout = setTimeout(() => {
    mouseTimeout = null;
  }, INTERACTION_CHECK_INTERVAL);

  // This only works if we have a particle data state,
  // since we can't relate unassociated hits.
  if (
    !Array.from(renderStates.values()).some(
      (state) => state.particleData.length !== 0,
    )
  )
    return [];

  mouse.x = (event.clientX / window.innerWidth) * 2 - 1;
  mouse.y = -(event.clientY / window.innerHeight) * 2 + 1;

  // Three possible highlight states:
  //  - Highlighting a single particle.
  //  - Highlighting a particle and any children. (Ctrl)
  //  - Highlighting a particles parent and all of the parent's children. (Shift)
  const shiftPressed = event.shiftKey;
  const ctrlPressed = event.ctrlKey;
  let selectedParticles = [];

  renderStates.forEach((state) => {
    if (!state.visible) {
      return;
    }

    raycaster.setFromCamera(mouse, state.camera);
    const intersects = raycaster.intersectObjects([state.hitGroup], true);

    // Find first valid intersection in one pass
    const intersectObject = intersects.find((intersect) => {
      return (
        intersect.object.type === "Mesh" &&
        intersect.object.material.opacity >= 0.75
      );
    });

    if (!intersectObject) return;

    const hitNum = intersectObject.instanceId;
    const activeHit = state.particleData.activelyDrawnHits[hitNum];
    if (!activeHit) return;

    const activeParticleId = state.particleData.hitToParticleMap.get(
      activeHit.id,
    );
    if (!activeParticleId) return;

    const activeParticle = state.particleData.particleMap.get(activeParticleId);
    if (!activeParticle) return;

    const parentParticle = state.particleData.getParent(activeParticle);
    const targetParticle = shiftPressed ? parentParticle : activeParticle;

    if (!targetParticle) return;

    selectedParticles.push(targetParticle.id);

    // If we're already highlighting this particle, don't do anything.
    if (currentlyHighlighting.includes(targetParticle.id)) {
      return;
    } else {
      state.particleData.disableHighlights();
    }

    // Include the child particles if shift or ctrl is pressed.
    if (ctrlPressed || shiftPressed) {
      targetParticle.childIDs.forEach((childId) => {
        state.particleData.addHighlightTarget(childId);
      });
    }

    state.particleData.addHighlightTarget(targetParticle.id);
    state.triggerEvent("fullUpdate");
  });

  if (currentlyHighlighting.length > 0 && selectedParticles.length === 0) {
    renderStates.forEach((state) => {
      if (!state.visible) {
        return;
      }
      state.particleData.disableHighlights();
      state.triggerEvent("fullUpdate");
    });
  }

  return selectedParticles;
}

/**
 * Sets up mouse-over interactions with camera movement tracking.
 * Disables highlighting during camera movement for better performance.
 *
 * @param {HTMLCanvasElement} canvas - The renderer canvas element
 * @param {Map<string, RenderState>} renderStates - The render states
 */
export function setupMouseOverInteractions(canvas, renderStates) {
  let currentlyHighlighting = [];
  let isMovingCamera = false;
  let movementTimeout;

  // Track camera movement to disable mouseovers during movement
  renderStates.forEach((state) => {
    state.controls.addEventListener("start", () => {
      isMovingCamera = true;
      // Clear any existing highlights when starting camera movement
      if (currentlyHighlighting.length > 0) {
        renderStates.forEach((renderState) => {
          if (renderState.visible) {
            renderState.particleData.disableHighlights();
            renderState.triggerEvent("fullUpdate");
          }
        });
        currentlyHighlighting = [];
      }
    });

    state.controls.addEventListener("end", () => {
      // Add small delay before re-enabling to avoid flickering
      clearTimeout(movementTimeout);
      movementTimeout = setTimeout(() => {
        isMovingCamera = false;
      }, 100);
    });
  });

  canvas.addEventListener("mousemove", (event) => {
    // Skip highlighting during camera movement
    if (isMovingCamera) return;

    currentlyHighlighting = highlightParticleOnMouseMove(
      renderStates,
      currentlyHighlighting,
      event,
    );
  });
}
