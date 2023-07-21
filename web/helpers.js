//
// Useful helper functions.
//

import * as THREE from "three";

import { BUTTON_ID } from "./constants.js";

/**
 * Returns an array containing the minimum and maximum values of a given
 * property in an array of objects.
 *
 * @param {Array} arr - The array of objects to search through.
 * @param {string} prop - The name of the property to search for.
 * @returns {Array} An array containing the minimum and maximum values of the given property.
 */
export function getMinMax(arr, prop) {
  return arr.reduce(
    (acc, value) => {
      return [Math.min(value[prop], acc[0]), Math.max(value[prop], acc[1])];
    },
    [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY],
  );
}

/**
 * Returns an object containing the center, width, minimum and maximum values of
 * a given axis in an array of hits.
 *
 * @param {Array} hits - The array of hits to search through.
 * @param {string} axis - The name of the axis to search for.
 * @returns {Object} An object containing the center, width, minimum and maximum values of the given axis.
 */
export function getHitBoundaries(hits, axis) {
  const minMax = getMinMax(hits, axis);

  const center = (minMax[0] + minMax[1]) / 2 ?? 0.0;
  const width = Math.abs(minMax[0] - minMax[1]);

  return { center: center, width: width, min: minMax[0], max: minMax[1] };
}

/**
 * Returns a map of hit properties for each hit in the given array of hits.
 *
 * @param {Array} hits - The array of hits to get properties for.
 * @returns {Map} A map of hit properties for each hit in the given array of hits.
 */
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

/**
 * Returns a map of hit classes for each hit in the given array of hits.
 *
 * @param {Array} hits - The array of hits to get classes for.
 * @returns {Map} A map of hit classes for each hit in the given array of hits.
 */
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

