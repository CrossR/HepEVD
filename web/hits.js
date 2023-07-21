//
// Hit-based functions.
//

import * as THREE from "three";
import { Lut } from "three/addons/math/Lut.js";

import { BUTTON_ID, materialHit } from "./constants.js";

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
) {

  if (hits.length === 0) return;

  // Check if we are using colour, and set it up if we are.
  const energyLut = new Lut("cooltowarm", 512);
  let usingColour = hitColours.length === hits.length;
  let usingLut = typeof hitColours[0] === 'number'

  if (usingColour && usingLut) {
    let minColourValue = Infinity;
    let maxColourValue = Number.NEGATIVE_INFINITY;
    hitColours.forEach((value) => {
      if (value < minColourValue) minColourValue = value;
      if (value > maxColourValue) maxColourValue = value;
    });
    energyLut.setMax(maxColourValue);

    if (maxColourValue === minColourValue) usingColour = false;
  }

  // Start building the mesh.
  const hitSize = hitConfig.hitSize;
  const hitGeometry = new THREE.BoxGeometry(hitSize, hitSize, hitSize);
  const dummyObject = new THREE.Object3D();
  const hitMesh = new THREE.InstancedMesh(hitGeometry, materialHit, hits.length);

  hits.forEach(function (hit, index) {

    dummyObject.position.set(hit.x, hit.y, hit.z);
    dummyObject.updateMatrix();

    hitMesh.setMatrixAt(index, dummyObject.matrix);

    if (usingColour && usingLut) {
      hitMesh.setColorAt(index, energyLut.getColor(hitColours[index]));
    } else if (usingColour && ! usingLut) {
      hitMesh.setColorAt(index, new THREE.Color(hitColours[index]));
    } else {
      hitMesh.setColorAt(index, new THREE.Color("gray"));
    }
  });

  hitMesh.instanceMatrix.needsUpdate = true;
  hitMesh.instanceColor.needsUpdate = true;

  group.add(hitMesh);
}
