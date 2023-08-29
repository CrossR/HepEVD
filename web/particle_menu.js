//
// Particle Info/Filtering Menu
//
// Display a nested menu of particle information, with checkboxes to
// filter the particles displayed in the main view.

import { INTERACTION_TYPE_SCORE } from "./constants.js";

function createMenuItem(
  particle,
  onClick,
  particlesMap,
  usedParticles,
  parentElement
) {
  usedParticles.add(particle.id);

  const menuItem = document.createElement("li");
  menuItem.classList.add("block");

  const details = document.createElement("details");
  details.open = false;

  const summary = document.createElement("summary");
  const label = document.createElement("span");
  label.innerHTML = `${particle.interactionType} (${particle.hits.length})`;
  label.classList.add("label-text");
  label.addEventListener("click", () => {
    onClick(particle, particlesMap, "ALL");
  });
  summary.appendChild(label);

  // Now, add the child elements for the particle
  const elementList = document.createElement("ul");
  ["Vertices", "Clusters", "Child PFOs"].forEach((childType) => {
    if (childType === "Child PFOs" && particle.childIDs.length > 0) {
      const details = document.createElement("details");
      details.open = false;

      const summary = document.createElement("summary");
      const label = document.createElement("span");
      label.innerHTML = childType;
      label.classList.add("label-text");
      label.addEventListener("click", () => {
        onClick(particle, particlesMap, childType);
      });
      summary.appendChild(label);
      details.appendChild(summary);

      particle.childIDs.map((childID) => {
        if (usedParticles.has(childID)) {
          return;
        }
        const particle = particlesMap.get(childID);

        // INFO: Likely a particle with no valid hits for this dimension.
        if (particle === undefined) {
          return;
        }

        createMenuItem(particle, onClick, particlesMap, usedParticles, details);
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

  details.appendChild(summary);
  details.appendChild(elementList);
  menuItem.appendChild(details);
  parentElement.appendChild(menuItem);
}

export function createParticleMenu(particlesMap, onClick) {
  const menu = document.getElementById("particle_menu");
  const usedParticles = new Set();

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
    createMenuItem(particle, onClick, particlesMap, usedParticles, menu);
  });
}
