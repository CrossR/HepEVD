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

        if (childType === "Child PFOs" && particle.childIDs.length > 0) {

            const details = document.createElement("details");
            details.open = false;

            const summary = document.createElement("summary");
            const checkbox = document.createElement("input");
            checkbox.type = "checkbox";
            checkbox.id = `${particle.id}_child_pfos`;
            checkbox.checked = true;
            checkbox.classList.add("checkbox", "checkbox-xs");
            summary.appendChild(checkbox);

            const label = document.createElement("span");
            label.innerHTML = childType;
            label.classList.add("label-text");
            summary.appendChild(label);

            details.appendChild(summary);

            particle.childIDs.map((childID) => {
                if (usedParticles.has(childID)) {
                    return;
                }
                const particle = particlesMap.get(childID);
                createMenuItem(particle, particlesMap, usedParticles, details);
            });

            elementList.appendChild(details);
            return;
        } else if (childType === "Child PFOs" && particle.childIDs.length === 0) {
            return;
        }

        const childItem = document.createElement("li");

        const childCheckbox = document.createElement("input");
        childCheckbox.type = "checkbox";
        childCheckbox.id = `${particle.id}_${childType.toLowerCase()}`;
        childCheckbox.checked = true;
        childCheckbox.classList.add("checkbox", "h-2", "w-2");
        childItem.appendChild(childCheckbox);

        const childLabel = document.createElement("span");
        childLabel.innerHTML = childType;
        childLabel.classList.add("label-text");
        childItem.appendChild(childLabel);

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

    // Sort the particles by the number of total hits, including children.
    const particles = Array.from(particlesMap.values()).sort((a, b) => {
        if (a.parentID !== "" || b.parentID !== "") {
            return 1;
        }
        const aHits = a.hits.length + a.childIDs.reduce((acc, childID) => {
            return acc + particlesMap.get(childID).hits.length;
        }, 0);
        const bHits = b.hits.length + b.childIDs.reduce((acc, childID) => {
            return acc + particlesMap.get(childID).hits.length;
        }, 0);

        return bHits - aHits;
    });

    particles.forEach((particle, _) => {
        createMenuItem(particle, particlesMap, usedParticles, menu);
    });
}