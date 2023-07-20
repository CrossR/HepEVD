//
// Hit Rendering functions.
//

import * as THREE from "three";
import { Lut } from "three/addons/math/Lut.js";

import { BUTTON_ID, materialHit } from "./constants.js";

// Draw an array of 3D hits to the screen, utilising an InstancedMesh for
// performance.
//
// Optionally, colour the hits based on some property value.
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

  if (usingColour) {
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

    if (usingColour) {
      hitMesh.setColorAt(index, energyLut.getColor(hitColours[index]));
    } else {
      hitMesh.setColorAt(index, new THREE.Color("gray"));
    }
  });

  hitMesh.instanceMatrix.needsUpdate = true;
  hitMesh.instanceColor.needsUpdate = true;

  group.add(hitMesh);
}

// Setup the correct controls per context.
export function setupControls(viewType, controls) {
  if (viewType === "3D") setupThreeDControls(controls);
  else setupTwoDControls(controls);
}

// Swap to 2D controls.
export function setupTwoDControls(controls) {
  controls.screenSpacePanning = true;
  controls.enableRotate = false;
  controls.mouseButtons = {
    LEFT: THREE.MOUSE.PAN,
    MIDDLE: THREE.MOUSE.DOLLY,
    RIGHT: null,
  };

  controls.update();
}

// Swap to 3D controls.
export function setupThreeDControls(controls) {
  controls.screenSpacePanning = true;
  controls.enableRotate = true;
  controls.mouseButtons = {
    LEFT: THREE.MOUSE.ROTATE,
    MIDDLE: THREE.MOUSE.DOLLY,
    RIGHT: THREE.MOUSE.PAN,
  };

  controls.update();
}
