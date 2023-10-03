//
// Useful helper functions.
//

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
 * @param {Array} particles - The array of particles to get properties for.
 * @param {Array} hits - The array of hits to get properties for.
 * @returns {Map} A map of hit properties for each hit in the given array of hits.
 */
export function getHitProperties(particles, hits) {
  const hitPropMaps = new Map();

  const allHits = particles.flatMap((particle) => {
    return particle.hits;
  });
  allHits.push(...hits);

  // Every hit should have an energy property, but there is
  // then two additional cases where a hit can be grouped:
  //  - If a hit is labelled.
  //  - If a hit has a property map associated.
  allHits.forEach((hit) => {
    hitPropMaps.set(hit, new Map([[BUTTON_ID.All, 0.0]]));
    hitPropMaps.get(hit).set("energy", hit.energy);

    if (hit.label !== "") {
      hitPropMaps.get(hit).set(hit.label, 1.0);
    }

    if (Object.keys(hit.properties).length > 0) {
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
 * @param {Array} particles - The array of particles to get properties for.
 * @param {Array} hits - The array of hits to get types for.
 * @returns {Map} A map of hit types for each hit in the given array of hits.
 */
export function getHitTypes(particles, hits) {
  const typeFilterMap = new Map();
  typeFilterMap.set(BUTTON_ID.All, (_) => {
    return true;
  });

  const allHits = particles.flatMap((particle) => {
    return particle.hits;
  });
  allHits.push(...hits);

  allHits.forEach((hit) => {
    if (hit.position.hitType !== "Hit") {
      const currentHitType = hit.position.hitType;
      typeFilterMap.set(currentHitType, (hit) => {
        return hit.position.hitType === currentHitType;
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
      console.log(`Found unknown PDG code: ${mcPdg}`);
    }
  });

  return mcHitColours;
}

/**
 * Calculates a hash value for the given string.
 *
 * @param {string} str - The string to hash.
 * @returns {number} The hash value for the string.
 */
export function hashStr(str) {
  let hash = 0;

  for (let i = 0; i < str.length; i++) {
    const char = str.charCodeAt(i);
    hash = (hash << 5) - hash + char;
    hash |= hash;
  }

  return hash;
}
