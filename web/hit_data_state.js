//
// Hit Data State
//

import { BUTTON_ID } from "./constants.js";
import { getHitProperties } from "./helpers.js";
import { HitTypeState } from "./hit_type_state.js";

export class HitDataState {
  constructor(particles, hits) {
    this.allHits = hits;

    this.activeHits = [];
    this.colours = [];

    const hitProperties = getHitProperties(particles, hits);
    this.props = hitProperties.hitPropMaps;
    this.propTypes = hitProperties.hitPropTypes;

    this.activeProps = new Set([BUTTON_ID.All]);

    this.active = true;
  }

  /**
   * Get the active hits;
   *
   * @returns {Array} The hits array.
   */
  get hits() {
    return this.active ? this.activeHits : [];
  }

  /**
   * Get all the hits.
   *
   * @returns {Array} The hits array.
   */
  get all() {
    return this.allHits;
  }

  /**
   * Gets the length of the hits array.
   *
   * @returns {number} The length of the hits array.
   */
  get length() {
    return this.hits.length;
  }

  /**
   * Toggles the hit property.
   *
   * @param {string} prop - The property to toggle.
   */
  toggleHitProperty(prop) {
    if (prop === BUTTON_ID.None) {
      this.activeProps.clear();
    } else {
      if (this.activeProps.has(prop)) {
        this.activeProps.delete(prop);
      } else {
        this.activeProps.add(prop);
      }
    }
  }

  /**
   * Toggles the active state of the hit data.
   *
   * @param {boolean} active - The active state.
   */
  toggleActive(active = !this.active) {
    this.active = active;
  }

  /**
   * Top level update function, to update what the active hits are.
   *
   * @param {Array} particles - The particles to consider for updating the active hits.
   * @param {HitTypeState} hitTypeState - The hit type state object.
   */
  updateActive(particles, hitTypeState) {
    let newHits = new Set();
    const newHitColours = [];

    this.allHits.forEach((hit) => {
      // Skip if hit type is not active
      if (!hitTypeState.checkHitType(hit)) return;

      // Otherwise, add it if it matches the active properties
      Array.from(this.activeProps)
        .reverse()
        .filter((prop) => prop !== BUTTON_ID.All)
        .forEach((prop) => {
          const propName = typeof prop === "string" ? prop : prop.args[0];
          const propFunc = typeof prop === "string" ? null : prop.func;
          if (!this.props.get(hit.id)) return;
          if (newHits.has(hit)) return;

          if (propFunc && !propFunc(prop.args, this.props.get(hit.id))) return;
          else if (!this.props.get(hit.id).has(propName)) return;

          newHits.add(hit);
          newHitColours.push(this.props.get(hit.id).get(propName));
        });

      // If we've added the hit, we're done.
      if (newHits.has(hit)) return;

      // Otherwise, check if the ALL button is active
      if (this.activeProps.has(BUTTON_ID.All)) {
        newHits.add(hit);
        newHitColours.push(this.props.get(hit.id).get(BUTTON_ID.All));
      }
    });

    // If we have zero hits at this point, lets just use the particles list to
    // build the hit list.
    if (newHits.size === 0 && particles) {
      newHits = particles.flatMap((p) => p.hits);
    }

    this.colours = newHitColours;
    this.activeHits = Array.from(newHits);
  }
}
