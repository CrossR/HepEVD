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
    this.highlightTargets = new Set();

    // Map from particle id to particle, as well as hit id to particle id.
    this.particleMap = new Map();
    this.hitToParticleMap = new Map();
    particles.forEach((particle) => {
      this.particleMap.set(particle.id, particle);
      particle.hits.forEach((hit) => {
        this.hitToParticleMap.set(hit.id, particle.id);
      });
    });

    // Map from child particles, to top level parent particle.
    this.childToParentMap = new Map();
    particles.forEach((particle) => {
      let currentParticle = particle;
      while (currentParticle.parentID !== "") {
        const parentParticle = this.particleMap.get(currentParticle.parentID);
        currentParticle = parentParticle;
      }

      this.childToParentMap.set(particle.id, currentParticle.id);
    });

    this.activelyDrawnHits = [];
    this.active = true;
  }

  /**
   * Get the particles from the state.
   *
   * @returns {Array} The particles.
   */
  get particles() {
    return this.active ? this.activeParticles : [];
  }

  /**
   * Get all the particles.
   *
   * @returns {Array} The particles.
   */
  get all() {
    return this.allParticles;
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
   * Get the parent particle for the given particle.
   * @param {Object} particle - The particle object to get the parent of.
   *
   * @returns {Object} - The parent particle, or the particle itself if it has no parent.
   */
  getParent(particle) {
    const parentID = this.childToParentMap.get(particle.id);

    if (parentID) return this.particleMap.get(parentID);

    return particle;
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
   * Toggles the active state of the particle data.
   *
   * @param {boolean} active - The active state.
   */
  toggleActive(active = !this.active) {
    this.active = active;
  }

  /**
   * Add a highlight target.
   *
   * @param {string} id - The target particle id.
   */
  addHighlightTarget(id) {
    this.highlightTargets.add(id);
  }

  /**
   * Remove a highlight target.
   *
   * @param {string} id - The target particle id.
   */
  removeHighlightTarget(id) {
    this.highlightTargets.delete(id);
  }

  /**
   * Disable highlighting.
   *
   */
  disableHighlights() {
    this.highlightTargets.clear();
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
    if (!this.active) {
      this.activeParticles = [];
      return;
    }

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
