//
// Hit-based functions.
//

import * as THREE from "three";
import { Lut } from "three/addons/math/Lut.js";

import { addColourMap, getCategoricalLutConf, getContinuousLutConf } from "./colourmaps.js";
import {
  materialHit,
} from "./constants.js";
import { hashStr } from "./helpers.js";

/**
 * Draws a set of hits as a 3D mesh using Three.js.
 *
 * @param {THREE.Group} group - The group to which the hit mesh should be added.
 * @param {Array} hits - An array of hit objects, each with an x, y, and z position.
 * @param {Array} hitColours - An array of colour values, one for each hit.
 * @param {Object} hitConfig - An object containing configuration options for the hit mesh.
 */
export function drawHits(
  group,
  hits,
  hitColours,
  hitConfig = {},
  lutConfig = getContinuousLutConf(),
) {
  if (hits.length === 0) return;

  // Check if we are using colour, and set it up if we are.
  const colourLut = new Lut("cooltowarm", 10);
  addColourMap(colourLut, lutConfig.name, lutConfig.size);
  let usingColour = hitColours.length === hits.length;
  let usingLut = typeof hitColours[0] === "number";

  if (usingColour && usingLut) {
    let minColourValue = Infinity;
    let maxColourValue = Number.NEGATIVE_INFINITY;
    hitColours.forEach((value) => {
      if (value < minColourValue) minColourValue = value;
      if (value > maxColourValue) maxColourValue = value;
    });

    // Set to the maximum value between the calcualted max, and the LUT default.
    colourLut.setMax(maxColourValue);
    colourLut.setMin(minColourValue);

    if (maxColourValue === minColourValue) usingColour = false;
  }

  // Start building the mesh.
  const hitSize = hitConfig.hitSize;
  const hitGeometry = new THREE.BoxGeometry(hitSize, hitSize, hitSize);
  const dummyObject = new THREE.Object3D();
  const hitMesh = new THREE.InstancedMesh(
    hitGeometry,
    materialHit,
    hits.length
  );

  hits.forEach(function (hit, index) {
    const pos = hit.position;
    dummyObject.position.set(pos.x, pos.y, pos.z);
    dummyObject.updateMatrix();

    hitMesh.setMatrixAt(index, dummyObject.matrix);

    if (usingColour && usingLut) {
      hitMesh.setColorAt(index, colourLut.getColor(hitColours[index]));
    } else if (usingColour && !usingLut) {
      hitMesh.setColorAt(index, new THREE.Color(hitColours[index]));
    } else {
      hitMesh.setColorAt(index, new THREE.Color(0x808080)); // Gray
    }
  });

  hitMesh.instanceMatrix.needsUpdate = true;
  hitMesh.instanceColor.needsUpdate = true;

  group.add(hitMesh);
}

/**
 * Draws particles on a given group element.
 *
 * @param {THREE.Group} group - The group to which the particles should be added.
 * @param {Array} particles - An array of particle objects, each with an array of hits.
 * @param {Array} activeHitProps - An array of active hit properties, used for colouring.
 * @param {Map} hitPropMap - A map from hit property names to their values.
 * @param {Object} hitConfig - An object containing configuration options for the hit mesh.
 */
export function drawParticles(
  group,
  particles,
  activeHitProps,
  hitPropMap,
  hitConfig
) {
  const hits = particles.map((particle) => {
    return particle.hits;
  });

  let lutToUse = getCategoricalLutConf();

  // Particle colour here is just their index in the array,
  // modulo the number of colours in the colour map.
  const particleColours = particles.flatMap((particle, _) => {
    return particle.hits.map((hit) => {
      if (activeHitProps.size > 1) {
        return Array.from(activeHitProps)
          .reverse()
          .map((prop) => {
            return hitPropMap.get(hit).get(prop);
          })[0];
      }

      return (
        Math.abs(hashStr(particle.id)) % lutToUse.size
      );
    });
  });

  if (activeHitProps.size > 1) {
    lutToUse = getContinuousLutConf();
  }

  drawHits(group, hits.flat(), particleColours, hitConfig, lutToUse);
}
