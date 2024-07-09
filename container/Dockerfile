FROM dmxbox/espidf

RUN : \
  && apt-get update \
  && apt-get install -y \
    eza \
    fzf \
    neovim \
    ripgrep \
    zsh \
  && apt-get autoremove -y \
  && rm -rf /var/lib/apt/lists/* \
  && sh -c "$(curl -fsLS https://starship.rs/install.sh)" -- --yes \
  && $IDF_PATH/tools/idf_tools.py --non-interactive install esp-clang \
  && rm -rf $IDF_TOOLS_PATH/dist \
  && :

USER ubuntu
ENV HOME /home/ubuntu
ENV ENV "${HOME}/.shrc"
ENV XDG_CACHE_HOME "${HOME}/.cache"
ENV XDG_CONFIG_HOME "${HOME}/.config"
ENV XDG_STATE_HOME "${HOME}/.local/state"
ENV ZDOTDIR "${XDG_CONFIG_HOME}/zsh"

WORKDIR "${HOME}"

RUN : \
  && sh -c "$(curl -fsLS get.chezmoi.io/lb)" -- init --apply juliekoubova \
  && export NVM_DIR="$HOME/.config/nvm" \
  && \. "$NVM_DIR/nvm.sh" \
  && nvm install 20 \
  && :

