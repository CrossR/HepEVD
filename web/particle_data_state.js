//
// Particle Data State
//

import { HitDataState } from "./hit_data_state.js";
import { HitTypeState } from "./hit_type_state.js";

export class ParticleDataState {
  constructor(particles) {
    this.allParticles = particles;

    this.activeParticles = [];
    this.activeInteractionTypes = new Set();
    this.ignoredParticles = new Set();

    // Map from particle id to particle, as well as hit id to particle id.
    this.particleMap = new Map();
    this.hitToParticleMap = new Map();
    this.particles.forEach((particle) => {
      this.particleMap.set(particle.id, particle);
      particle.hits.forEach((hit) => {
        this.hitToParticleMap.set(hit.id, particle.id);
      });
    });

    // Map from child particles, to top level parent particle.
    this.childToParentMap = new Map();
    this.particles.forEach((particle) => {
      let currentParticle = particle;
      while (currentParticle.parentID !== "") {
        const parentParticle = this.particleMap.get(currentParticle.parentID);
        currentParticle = parentParticle;
      }

      this.childToParentMap.set(particle, currentParticle);
    });
  }

  /**
   * Get the particles from the state.
   *
   * @returns {Array} The particles.
   */
  get particles() {
    return this.activeParticles.length > 0
      ? this.activeParticles
      : this.allParticles;
  }

  /**
   * Get the number of particles in the state.
   *
   * @returns {number} The number of particles.
   */
  get length() {
    return this.particles.length;
  }

  /**
   * Checks if a particle is ignored.
   * @param {Object} particle - The particle object to check.
   *
   * @returns {boolean} - True if the particle is ignored, false otherwise.
   */
  checkIgnored(particle) {
    return this.ignoredParticles.has(particle.id);
  }

  /**
   * Toggles the interaction type.
   *
   * @param {string} type - The interaction type to toggle.
   */
  toggleInteractionType(type) {
    if (this.activeInteractionTypes.has(type)) {
      this.activeInteractionTypes.delete(type);
    } else {
      this.activeInteractionTypes.add(type);
    }
  }

  /**
   * Ignores a particle and its children, if specified.
   *
   * @param {Object} particle - The particle object to ignore.
   * @param {boolean} [withChildren=true] - Flag indicating whether to ignore the particle's children as well. Default is true.
   */
  ignoreParticle(particle, withChildren = true) {
    this.ignoredParticles.add(particle.id);

    if (withChildren) {
      particle.childIDs.map((childId) => {
        this.ignoredParticles.add(childId);
      });
    }
  }

  /**
   * Removes the specified particle from the ignoredParticles set.
   *
   * @param {Object} particle - The particle to be unignored.
   * @param {boolean} [withChildren=true] - Indicates whether to unignore the children of the particle.
   */
  unignoreParticle(particle, withChildren = true) {
    this.ignoredParticles.delete(particle.id);

    if (withChildren) {
      particle.childIDs.map((childId) => {
        this.ignoredParticles.delete(childId);
      });
    }
  }

  /**
   * Checks if a particle is valid based on the active interaction types and ignored particles.
   *
   * @param {Object} particle - The particle object to be checked.
   * @returns {boolean} - Returns true if the particle is valid, false otherwise.
   */
  checkParticleIsValid(particle) {
    if (
      this.activeInteractionTypes.size > 0 &&
      !this.activeInteractionTypes.has(particle.interactionType)
    )
      return false;

    if (this.ignoredParticles.has(particle.id)) return false;

    return true;
  }

  /**
   * Top level function to update the active particles.
   *
   * @param {HitDataState} hitData - To utilise the activeProps and props properties.
   * @param {HitTypeState} hitTypeState - To utilise the checkHitType function>>.
   */
  updateActive(hitData, hitTypeState) {
    const newParticles = this.allParticles.flatMap((particle) => {
      if (!this.checkParticleIsValid(particle)) return [];

      const newParticle = { ...particle };

      newParticle.hits = particle.hits.filter((hit) => {
        if (!hitTypeState.checkHitType(hit)) return false;

        return Array.from(hitData.activeProps).some((prop) => {
          return hitData.props.get(hit.id).has(prop);
        });
      });

      if (newParticle.hits.length === 0) return [];

      return newParticle;
    });

    this.activeParticles = newParticles;
  }
}
