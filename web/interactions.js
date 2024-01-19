//
// Interactions
//

import * as THREE from "three";

import { HIT_CONFIG } from "./constants.js";
import { drawParticleOverlay } from "./hits.js";

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

  let selectedParticles = [];

  renderStates.forEach((state) => {
    if (!state.visible) {
      return;
    }

    const raycaster = new THREE.Raycaster();
    raycaster.setFromCamera(mouse, state.camera);
    const intersects = raycaster.intersectObjects(state.scene.children, true);
    const visibleIntersects = intersects.filter((intersect) => {
      return intersect.object.material.opacity > 0.75;
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

      if (!parentParticle) return;

      selectedParticles.push(parentParticle.id);

      // If we're already highlighting this particle, don't do anything.
      if (currentlyHighlighting.includes(parentParticle.id)) {
        return;
      }

      // If we are highlighting a new particle, we need to clear the old one.
      if (currentlyHighlighting.find((id) => id !== parentParticle.id)) {
        state.triggerEvent("fullUpdate");
      }

      // Finally, lets render out all the hits of this particle, but with a unique glow.
      drawParticleOverlay(
        state.hitGroup,
        state.particleData,
        state.hitData,
        state.hitTypeState,
        HIT_CONFIG[state.hitDim],
        parentParticle,
      );

      state.triggerEvent("change");
    }
  });

  if (currentlyHighlighting.length > 0 && selectedParticles.length === 0) {
    renderStates.forEach((state) => {
      if (!state.visible) {
        return;
      }
      state.triggerEvent("fullUpdate");
    });
  }

  return selectedParticles;
}
