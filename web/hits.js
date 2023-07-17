//
// Hit Rendering functions.
//

import * as THREE from "three";
import { Lut } from "three/addons/math/Lut.js";

import { BUTTON_ID } from "./constants.js";

// Draw an array of 3D hits to the screen, utilising an InstancedMesh for
// performance.
//
// Optionally, colour the hits based on some property value.
export function drawHits(
  group,
  material,
  activeHits,
  hitPropMap,
  useColour = false,
  hitConfig = {},
  activeHitFilter = (_) => { return true; },
) {
  // Produce arrays containing all the input hits, and the required
  // hit properties.
  const allColourProps = [...activeHits.keys()];
  const renderingAll = allColourProps.includes(BUTTON_ID.All);
  const hits = renderingAll
    ? activeHits.get(BUTTON_ID.All)
    : [...activeHits.values()].flat();

  if (hits.length === 0) return;

  // Setup some basic THREE js properties for later use.
  const hitSize = hitConfig.hitSize;
  const hitGeometry = new THREE.BoxGeometry(hitSize, hitSize, hitSize);

  const dummyObject = new THREE.Object3D();
  const energyLut = new Lut("rainbow", 512);

  // Build up an easily parseable map of hit -> property to use for rendering.
  const properties = new Map();
  let numberOfHits = 0;

  hits.forEach((hit, index) => {
    if (!hitPropMap.has(hit)) {
      return;
    }

    numberOfHits += 1;

    allColourProps.forEach((colourProp) => {
      if (!hitPropMap.get(hit).has(colourProp)) {
        return;
      }

      // TODO: Need to decide the best way to pick which property to use if
      //       there are multiple. Right now, its always latest.
      const hitProp = hitPropMap.get(hit).get(colourProp);
      properties.set(index, hitProp);
    });
  });

  let usingColour =
    useColour &&
    properties.size > 0 &&
    [...properties.values()][0].constructor === Number;
  const usingProperties = properties.size > 0;

  if (usingColour) {
    let minColourValue = Infinity;
    let maxColourValue = Number.NEGATIVE_INFINITY;
    properties.forEach((value, _) => {
      if (value < minColourValue) minColourValue = value;
      if (value > maxColourValue) maxColourValue = value;
    });
    energyLut.setMax(maxColourValue);

    if (maxColourValue === minColourValue) usingColour = false;
  }

  // Finally, start building the mesh.
  const hitMesh = new THREE.InstancedMesh(hitGeometry, material, numberOfHits);

  hits.forEach(function (hit, index) {

    // Don't render if we are missing the property.
    if (usingProperties && !properties.has(index)) {
      return;
    }

    // Don't render if its being skipped by the active filter.
    if (!activeHitFilter(hit)) {
      return;
    }

    dummyObject.position.set(hit.x, hit.y, hit.z);
    dummyObject.updateMatrix();

    hitMesh.setMatrixAt(index, dummyObject.matrix);

    if (usingColour) {
      hitMesh.setColorAt(index, energyLut.getColor(properties.get(index)));
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
