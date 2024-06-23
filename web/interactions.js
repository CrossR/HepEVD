//
// Interactions
//

import * as THREE from "three";

export function highlightParticleOnMouseMove(
  renderStates,
  currentlyHighlighting,
  event,
) {
  // This only works if we have a particle data state,
  // since we can't relate unassociated hits.
  if (
    !Array.from(renderStates.values()).some(
      (state) => state.particleData.length !== 0,
    )
  )
    return [];

  const mouse = new THREE.Vector2();
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

    const raycaster = new THREE.Raycaster();
    raycaster.setFromCamera(mouse, state.camera);
    const intersects = raycaster.intersectObjects(state.scene.children, true);
    const visibleIntersects = intersects.filter((intersect) => {
      return intersect.object.material.opacity >= 0.75;
    });
    if (visibleIntersects.length > 0) {
      const intersectObject = visibleIntersects[0];

      if (intersectObject.object.type !== "Mesh") {
        return;
      }

      const hitNum = intersectObject.instanceId;
      const activeHit = state.particleData.activelyDrawnHits[hitNum];
      let activeParticle;

      try {
        const activeParticleId = state.particleData.hitToParticleMap.get(
          activeHit.id,
        );
        activeParticle = state.particleData.particleMap.get(activeParticleId);
      } catch {
        return;
      }

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
        targetParticle.childIDs.map((childId) => {
          state.particleData.addHighlightTarget(childId);
        });
      }

      state.particleData.addHighlightTarget(targetParticle.id);
      state.triggerEvent("fullUpdate");
    }
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
