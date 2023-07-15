//
// Useful helper functions.
//

import * as THREE from "three";

import { BUTTON_ID } from "./constants.js";

// Convert a JS position object to threejs.
export function positionToVector(position) {
  return new THREE.Vector3(position.x, position.y, position.z);
}

export function getMinMax(arr, prop) {
  return arr.reduce(
    (acc, value) => {
      return [Math.min(value[prop], acc[0]), Math.max(value[prop], acc[1])];
    },
    [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY],
  );
}

export function getHitBoundaries(hits, axis) {
  const minMax = getMinMax(hits, axis);

  const center = (minMax[0] + minMax[1]) / 2 ?? 0.0;
  const width = Math.abs(minMax[0] - minMax[1]);

  return { center: center, width: width, min: minMax[0], max: minMax[1] };
}

// Build up a map between a property name and a property for
// each sort of hit (3D, 2D).
//
// This should mean that any property can be easily drawn without
// needing to worry about the specific location of a property.
export function getHitProperties(hits) {
  const hitPropMaps = new Map();

  hitPropMaps.set("3D", new Map());
  hitPropMaps.set("2D", new Map());

  // Every hit should have an energy property, but there is
  // then two additional cases where a hit can be grouped:
  //  - If a hit is labelled.
  //  - If a hit has a property map associated.
  hits.forEach((hit) => {
    hitPropMaps.get(hit.type).set(hit, new Map([[BUTTON_ID.All, 0.0]]));
    hitPropMaps.get(hit.type).get(hit).set("energy", hit.energy);

    if (Object.hasOwn(hit, "label")) {
      hitPropMaps.get(hit.type).get(hit).set(hit.label, 1.0);
    }

    if (Object.hasOwn(hit, "properties")) {
      hit.properties.forEach((prop) => {
        const key = Object.keys(prop)[0];
        const value = Object.values(prop)[0];

        hitPropMaps.get(hit.type).get(hit).set(key, value);
      });
    }
  });

  return hitPropMaps;
}
