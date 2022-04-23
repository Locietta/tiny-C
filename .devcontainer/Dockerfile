FROM Locietta/loia-dev-container:latest
ENV CLASSPATH=.:/usr/share/java/antlr-complete.jar

RUN pacman -Syy --noconfirm antlr4 antlr4-runtime\
    && pacman -Scc --noconfirm && rm -rf /var/cache/pacman/pkg/*
