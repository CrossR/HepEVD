//
// Particle Info/Filtering Menu
//
// Display a nested menu of particle information, with checkboxes to
// filter the particles displayed in the main view.

function createMenuItem(particle, particlesMap, usedParticles, parentElement) {

    usedParticles.add(particle.id);

    const menuItem = document.createElement("li");

    const details = document.createElement("details");
    details.open = false;

    const summary = document.createElement("summary");

    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.id = particle.id;
    checkbox.checked = true;
    checkbox.classList.add("checkbox", "checkbox-xs");
    summary.appendChild(checkbox);

    const label = document.createElement("span");
    label.innerHTML = particle.interactionType;
    label.classList.add("label-text");
    summary.appendChild(label);

    // Now, add the child elements for the particle
    const elementList = document.createElement("ul");
    ["Vertices", "Clusters", "Child PFOs"].forEach((childType) => {

        const childItem = document.createElement("li");

        const childCheckbox = document.createElement("input");
        childCheckbox.type = "checkbox";
        childCheckbox.id = `${particle.id}_${childType.toLowerCase()}`;
        childCheckbox.checked = true;
        childCheckbox.classList.add("checkbox", "checkbox-xs");
        childItem.appendChild(childCheckbox);

        const childLabel = document.createElement("span");
        childLabel.innerHTML = childType;
        childLabel.classList.add("label-text");
        childItem.appendChild(childLabel);

        if (childType === "Child PFOs") {
            const childList = document.createElement("ul");
            particle.childIDs.map((childID) => {
                if (usedParticles.has(childID)) {
                    return;
                }
                const particle = particlesMap.get(childID);
                console.log(`Adding child ${particle.id} to ${particle.parentID}`)
                createMenuItem(particle, particlesMap, usedParticles, childList);
            });
            childItem.appendChild(childList);
        }

        elementList.appendChild(childItem);
    });

    details.appendChild(summary);
    details.appendChild(elementList);
    menuItem.appendChild(details);
    parentElement.appendChild(menuItem);
}

export function createParticleMenu(particlesMap) {

    const menu = document.getElementById("particle_menu");
    const usedParticles = new Set();

    console.log(`There are ${particlesMap.size} particles in the event.`)

    particlesMap.forEach((particle, _) => {
        createMenuItem(particle, particlesMap, usedParticles, menu);
    });
}