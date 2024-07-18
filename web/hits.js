//
// Hit-based functions.
//

import * as THREE from "three";
import { Lut } from "three/addons/math/Lut.js";

import * as BufferGeometryUtils from "three/addons/utils/BufferGeometryUtils.js";
import {
  addColourMap,
  getCategoricalLutConf,
  getContinuousLutConf,
} from "./colourmaps.js";
import {
  BUTTON_ID,
  materialHit,
  materialParticle,
  trackLineMaterial,
} from "./constants.js";
import { HitDataState } from "./hit_data_state.js";
import { ParticleDataState } from "./particle_data_state.js";

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
  let usingColour = hitColours.length === hits.length;
  let usingLut = typeof hitColours[0] === "number";

  const colourLut = new Lut("cooltowarm", 10);
  addColourMap(colourLut, lutConfig.name, lutConfig.size);

  if (usingColour && usingLut && lutConfig.style !== "categorical") {
    let minColourValue = Infinity;
    let maxColourValue = Number.NEGATIVE_INFINITY;
    hitColours.forEach((value) => {
      if (value < minColourValue) minColourValue = value;
      if (value > maxColourValue) maxColourValue = value;
    });

    // Set to the maximum value between the calculated max, and the LUT default.
    colourLut.setMax(maxColourValue);
    colourLut.setMin(minColourValue);

    if (maxColourValue === minColourValue) usingColour = false;
  }

  // Start building the mesh.
  const hitSize = hitConfig.hitSize;
  const hitGeometry = new THREE.BoxGeometry(hitSize, hitSize, hitSize);
  const dummyObject = new THREE.Object3D();
  const hitMaterial = hitConfig.materialHit ?? materialHit;
  const hitMesh = new THREE.InstancedMesh(
    hitGeometry,
    hitMaterial,
    hits.length,
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
  hitMesh.matrixAutoUpdate = false;

  group.add(hitMesh);
}

/**
 * Draws particles on a given group element.
 *
 * @param {THREE.Group} group - The group to which the particles should be added.
 * @param {ParticleDataState} particleDataState - All the particle objects, to find absolute positions for colouring.
 * @param {HitDataState} hitDataState - An array of active hit properties, used for colouring.
 * @param {Object} hitConfig - An object containing configuration options for the hit mesh.
 */
export function drawParticles(
  group,
  particleDataState,
  hitDataState,
  hitConfig,
) {
  const particles = particleDataState.allParticles;
  const activeParticles = particleDataState.particles;
  const activeHitProps = hitDataState.activeProps;
  const hitPropMap = hitDataState.props;

  // Build up a map of particle to absolute index.
  // This lets the colouring be consistent regardless of
  // the currently applied filters.
  const absoluteIndices = new Map();
  particles.forEach((particle, index) => {
    absoluteIndices.set(particle.id, index);
  });

  const hits = activeParticles.map((particle) => {
    return particle.hits;
  });
  particleDataState.activelyDrawnHits = hits.flat();

  let lutToUse = getCategoricalLutConf();
  const filteredActiveHitProps = Array.from(activeHitProps).filter(
    (p) => p != BUTTON_ID.All,
  );

  // Swap to a continuous LUT if we have active hit properties.
  if (filteredActiveHitProps.length > 0) {
    lutToUse = getContinuousLutConf();
  }

  // Setup the colour map, incase we need it.
  const colourLut = new Lut("cooltowarm", 10);
  addColourMap(colourLut, lutToUse.name, lutToUse.size);

  // Particle colour is based on the absolute index of the particle, modulo the LUT size.
  // If there are multiple active hit properties, use that instead.
  const particleColours = activeParticles.flatMap((particle, _) => {
    return particle.hits.map((hit) => {
      if (
        particleDataState.highlightTargets.size > 0 &&
        !particleDataState.highlightTargets.has(particle.id)
      ) {
        return "Grey";
      }
      if (filteredActiveHitProps.length > 0) {
        return Array.from(filteredActiveHitProps)
          .reverse()
          .map((prop) => {
            return hitPropMap.get(hit.id).get(prop);
          })[0];
      }

      return colourLut.getColor(
        absoluteIndices.get(particle.id) % lutToUse.size,
      );
    });
  });

  // Set the hit config material to the particle material.
  hitConfig.materialHit = hitConfig.materialParticle ?? materialParticle;

  // Also update the hit opacity if we are highlighting particles.
  if (particleDataState.highlightTargets.size > 0) {
    hitConfig.materialHit.opacity = 0.75;
  }

  // We can now finally draw the hits.
  drawHits(group, hits.flat(), particleColours, hitConfig, lutToUse);

  // Do any final drawing of additional, particle-level properties.
  const trackParticles = activeParticles.filter(
    (particle) => particle.renderType === "Track",
  );
  if (trackParticles.length > 0) {
    drawTracks(group, trackParticles, hitConfig);
  }
}

/**
 * Draws tracks on a given group element.
 *
 * @param {THREE.Group} group - The group to which the particles should be added.
 * @param {Array} particles - An array of particle objects, each with an array of hits.
 */
export function drawTracks(group, particles) {
  if (particles.length === 0) return;

  // Store all the tube geometries for a later merge.
  const geometries = [];

  // Start building the lines between the hits.
  particles.forEach((particle) => {
    // INFO: Plot the tracks as a Tube, not a line.  This is because we can't
    // use the Line class, as we can't change the thickness due to a platform
    // limitation.
    const path = new THREE.CatmullRomCurve3(
      particle.hits.map(
        (hit) =>
          new THREE.Vector3(hit.position.x, hit.position.y, hit.position.z),
      ),
    );
    const geo = new THREE.TubeGeometry(path, particle.hits.length, 2, 4, false);
    geometries.push(geo);
  });

  // Finally, merge and add to group.
  const mergedGeo = BufferGeometryUtils.mergeGeometries(geometries);
  const mesh = new THREE.Mesh(mergedGeo, trackLineMaterial);

  group.add(mesh);
}
