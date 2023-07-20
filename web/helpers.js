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
// each sort of hit.
//
// This should mean that any property can be easily drawn without
// needing to worry about the specific location of a property.
export function getHitProperties(hits) {
  const hitPropMaps = new Map();

  // Every hit should have an energy property, but there is
  // then two additional cases where a hit can be grouped:
  //  - If a hit is labelled.
  //  - If a hit has a property map associated.
  hits.forEach((hit) => {
    hitPropMaps.set(hit, new Map([[BUTTON_ID.All, 0.0]]));
    hitPropMaps.get(hit).set("energy", hit.energy);

    if (Object.hasOwn(hit, "label")) {
      hitPropMaps.get(hit).set(hit.label, 1.0);
    }

    if (Object.hasOwn(hit, "properties")) {
      hit.properties.forEach((prop) => {
        const key = Object.keys(prop)[0];
        const value = Object.values(prop)[0];

        hitPropMaps.get(hit).set(key, value);
      });
    }
  });

  return hitPropMaps;
}

// Build up a map between a hit class and a filter for that class.
export function getHitClasses(hits) {
  const classFilterMap = new Map();
  classFilterMap.set(BUTTON_ID.All, (_) => {
    return true;
  });

  hits.forEach((hit) => {
    if (Object.hasOwn(hit, "class") && hit.class !== "General") {
      const currentHitClass = hit.class;
      classFilterMap.set(hit.class, (hit) => {
        return hit.class === currentHitClass;
      });
    }
  });

  return classFilterMap;
}

// Initialise the default hit filters -> Nothing! Just always return true.
export function getDefaultFilters(classFilters) {
  const activeHitFilterMap = new Map();

  activeHitFilterMap.set("3D", [classFilters.get("3D").get(BUTTON_ID.All)]);
  activeHitFilterMap.set("2D", [classFilters.get("2D").get(BUTTON_ID.All)]);

  return activeHitFilterMap;
}

// Update the hit filter array, adding or removing the default "Show all"
// filter as needed.
export function fixFilters(currentFilters, classFilterMap, hitsAdded) {
  // Default all filter is present, and shouldn't be.
  if (hitsAdded) {
    const index = currentFilters.indexOf(classFilterMap.get(BUTTON_ID.All));
    if (index === -1) return;
    currentFilters.splice(index, 1);
  } else {
    // Default all filter is missing! Add it.
    currentFilters.push(classFilterMap.get(BUTTON_ID.All));
  }

  return;
}
