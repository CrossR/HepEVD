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

    this.props = getHitProperties(particles, hits);
    this.activeProps = new Set([BUTTON_ID.All]);
  }

  /**
   * Get the active hits, either the activeHits array or the allHits array.
   *
   * @returns {Array} The hits array.
   */
  get hits() {
    return this.activeHits.length > 0 ? this.activeHits : this.allHits;
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
          if (!this.props.get(hit.id)) return;
          if (!this.props.get(hit.id).has(prop)) return;
          if (newHits.has(hit)) return;

          newHits.add(hit);
          newHitColours.push(this.props.get(hit.id).get(prop));
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
