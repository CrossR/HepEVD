//
// Particle Info/Filtering Menu
//
// Display a nested menu of particle information, with checkboxes to
// filter the particles displayed in the main view.

import { INTERACTION_TYPE_SCORE } from "./constants.js";

function createMenuItem(
  hitDim,
  particle,
  onClick,
  particlesMap,
  parentElement
) {

  const menuItem = document.createElement("li");
  menuItem.id = `particle_${particle.id}_${hitDim}`;
  menuItem.classList.add("block");

  const details = document.createElement("details");
  details.open = false;

  const summary = document.createElement("summary");
  const label = document.createElement("span");
  label.classList.add("label-text", "pr-4");
  label.addEventListener("click", () => {
    onClick(particle, particlesMap, "All");
  });

  // Now, add the child elements for the particle
  // Add a new label for each of the child elements.
  // In the case of the "Child PFOs" element, we need to
  // recursively call this function to add the child particles
  // to the menu.
  let totalNumHits = particle.hits.length;
  const elementList = document.createElement("ul");
  ["Vertices", "Clusters", "Child PFOs"].forEach((childType) => {
    if (childType === "Child PFOs" && particle.childIDs.length > 0) {
      const details = document.createElement("details");
      details.open = false;

      const summary = document.createElement("summary");
      const childLabel = document.createElement("span");
      childLabel.innerHTML = childType;
      childLabel.classList.add("label-text");
      childLabel.addEventListener("click", () => {
        onClick(particle, particlesMap, childType);
      });
      summary.appendChild(childLabel);
      details.appendChild(summary);

      particle.childIDs.map((childID) => {
        const childParticle = particlesMap.get(childID);

        // INFO: Likely a particle with no valid hits for this dimension.
        if (childParticle === undefined) {
          return;
        }

        createMenuItem(
          hitDim,
          childParticle,
          onClick,
          particlesMap,
          details
        );

        // Lets also update the parent label to include the number of child
        // particle hits.
        totalNumHits += childParticle.hits.length;
      });

      elementList.appendChild(details);
      return;
    } else if (childType === "Child PFOs" && particle.childIDs.length === 0) {
      return;
    }

    const childItem = document.createElement("li");
    const childLabel = document.createElement("span");
    childLabel.innerHTML = childType;
    childLabel.classList.add("label-text");
    childLabel.addEventListener("click", () => {
      onClick(particle, particlesMap, childType);
    });
    childItem.appendChild(childLabel);
    elementList.appendChild(childItem);
  });

  // Finally set the label to include the total number of hits.
  label.innerHTML = `${particle.interactionType} (${totalNumHits})`;
  summary.appendChild(label);

  // Then add the summary + dropdown bits to the new Particle...
  details.appendChild(summary);
  details.appendChild(elementList);

  // And finally add the new Particle to the menu.
  menuItem.appendChild(details);
  parentElement.appendChild(menuItem);
}

export function createParticleMenu(hitDim, particlesMap, onClick) {
  const menu = document.getElementById(`particle_menu_${hitDim}`);

  // Filter then sort the particles. Filtering is used to
  // remove the child particles from the list, as they will
  // be included as sub-items of their parent particles.
  //
  // Sorting is done by:
  // 1. The interaction type, based on the INTERACTION_TYPE_SCORE.
  // 2. The number of hits (including child particles)
  const particles = Array.from(particlesMap.values())
    .filter((particle) => particle.parentID === "")
    .sort((a, b) => {
      if (a.interactionType !== b.interactionType) {
        return (
          INTERACTION_TYPE_SCORE[a.interactionType] -
          INTERACTION_TYPE_SCORE[b.interactionType]
        );
      }

      const aNumHits = a.childIDs.reduce((acc, childID) => {
        return acc + particlesMap.get(childID).hits.length;
      }, a.hits.length);
      const bNumHits = b.childIDs.reduce((acc, childID) => {
        return acc + particlesMap.get(childID).hits.length;
      }, b.hits.length);

      return aNumHits < bNumHits;
    });

  particles.forEach((particle, _) => {
    createMenuItem(hitDim, particle, onClick, particlesMap, menu);
  });
}
