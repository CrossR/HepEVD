//
// Interactions
//

import * as THREE from "three";

import { HIT_CONFIG } from "./constants.js";
import { drawParticleOverlay } from "./hits.js";

export function highlightParticleOnMouseMove(renderStates, currentlyHighlighting, event) {

  const mouse = new THREE.Vector2();
  mouse.x = (event.clientX / window.innerWidth) * 2 - 1;
  mouse.y = -(event.clientY / window.innerHeight) * 2 + 1;

  let particleSelected = false;

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

      if (! parentParticle) return;

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
      particleSelected = true;
    }
  });

  if (currentlyHighlighting && ! particleSelected) {
    renderStates.forEach((state) => {
      if (!state.visible) {
        return;
      }
      state.triggerEvent("fullUpdate");
    });
  }

  return particleSelected;
}
