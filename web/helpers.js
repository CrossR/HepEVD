//
// Useful helper functions.
//

import * as THREE from "three";

import { BUTTON_ID, PDG_TO_COLOUR } from "./constants.js";

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
  const positions = hits.map((hit) => {
    return hit.position;
  });
  const minMax = getMinMax(positions, axis);

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
      Object.entries(hit.properties).forEach((prop) => {
        hitPropMaps.get(hit).set(prop[0], prop[1]);
      });
    }
  });

  return hitPropMaps;
}

/**
 * Returns a map of hit types for each hit in the given array of hits.
 *
 * @param {Array} hits - The array of hits to get types for.
 * @returns {Map} A map of hit types for each hit in the given array of hits.
 */
export function getHitTypes(hits) {
  const typeFilterMap = new Map();
  typeFilterMap.set(BUTTON_ID.All, (_) => {
    return true;
  });

  hits.forEach((hit) => {
    if (Object.hasOwn(hit, "type") && hit.type !== "General") {
      const currentHitType = hit.type;
      typeFilterMap.set(hit.type, (hit) => {
        return hit.type === currentHitType;
      });
    }
  });

  return typeFilterMap;
}

/**
 * Get an array of colours for MC hits based on their PDG code.
 *
 * @param {Array} mcHits - An array of hit objects, with an associated PDG.
 * @returns {Array} An array of colour strings, to be used in the hit rendering.
 */
export function getMCColouring(mcHits) {
  let mcHitColours = [];

  mcHits.forEach((hit) => {
    const mcPdg = hit.properties["PDG"];
    if (Object.hasOwn(PDG_TO_COLOUR, mcPdg)) {
      mcHitColours.push(PDG_TO_COLOUR[mcPdg]);
    } else {
      console.log(mcPdg);
    }
  });

  return mcHitColours;
}
