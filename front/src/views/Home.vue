<template>
  <v-container>
    <v-layout text-xs-center wrap>
      <v-flex xs12 sm6 offset-sm3>
        <v-card>
          <!-- <v-card-title primary-title>
            <div class="ma-auto">
              <span class="grey--text">IDF version: {{version}}</span>
              <br>
              <span class="grey--text">ESP cores: {{cores}}</span>
            </div>
          </v-card-title> -->

          <v-card-text>
            <v-container fluid grid-list-lg>
              <v-layout row wrap>
                <v-flex xs9>
                  <v-text-field
                    v-model="config.ap.ssid"
                    :rules="[rules.required]"
                    counter
                    maxlength="32"
                    label="AP SSID"
                  ></v-text-field>
                </v-flex>
                <v-flex xs9>
                  <v-text-field
                    v-model="config.ap.password"
                    :rules="[rules.required]"
                    :disabled="config.ap.auth_mode == 'open'"
                    counter
                    maxlength="64"
                    label="AP Password"
                  ></v-text-field>
                </v-flex>
                <v-flex xs9>
                  <v-select
                    v-model="config.ap.auth_mode"
                    :rules="[rules.required]"
                    :items="ap_auth_modes"
                    label="AP auth mode"
                  ></v-select>
                </v-flex>
                <v-flex xs9>
                  <v-select v-model="config.ap.channel" :rules="[rules.required]" :items="channels"
                    :hint="config.sta.enabled ? 'Ignored when connected to existing Wi-Fi' : ''" persistent-hint
                    label="Channel"></v-select>
                </v-flex>
                <v-flex xs9>
                  <v-checkbox
                    v-model="config.sta.enabled"
                    label="Also connect to existing Wi-Fi"
                  ></v-checkbox>
                </v-flex>
                <v-flex xs9>
                  <v-text-field
                    v-model="config.sta.ssid"
                    :disabled="!config.sta.enabled"
                    :rules="config.sta.enabled ? [rules.required] : []"
                    counter
                    maxlength="32"
                    label="Existing Wi-Fi SSID"
                  ></v-text-field>
                </v-flex>
                <v-flex xs9>
                  <v-text-field
                    v-model="config.sta.password"
                    :disabled="!config.sta.enabled || config.sta.auth_mode == 'open'"
                    :rules="config.sta.enabled ? [rules.required] : []"
                    counter
                    maxlength="64"
                    label="Existing Wi-Fi password"
                  ></v-text-field>
                </v-flex>
                <v-flex xs9>
                  <v-select v-model="config.sta.auth_mode" :disabled="!config.sta.enabled"
                    :rules="config.sta.enabled ? [rules.required] : []" :items="sta_auth_modes"
                    label="Existing Wi-Fi auth mode"></v-select>
                </v-flex>
              </v-layout>
            </v-container>
          </v-card-text>
          <v-btn fab dark large color="red accent-4" @click="save_config">
            <v-icon dark>check_box</v-icon>
          </v-btn>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      config: { ap: {}, sta: {} },
      ap_auth_modes: [
        { text: "open", value: "open" },
        { text: "WEP", value: "WEP" },
        { text: "WPA", value: "WPA_PSK" },
        { text: "WPA2", value: "WPA2_PSK" },
        { text: "WPA+WPA2", value: "WPA_WPA2_PSK" },
        { text: "WPA3", value: "WPA3_PSK" },
        { text: "WPA2+WPA3", value: "WPA2_WPA3_PSK" },
      ],
      sta_auth_modes: [
        { text: "open", value: "open" },
        { text: "WEP", value: "WEP" },
        { text: "WPA", value: "WPA_PSK" },
        { text: "WPA2", value: "WPA2_PSK" },
        { text: "WPA3", value: "WPA3_PSK" },
      ],
      channels: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12],
      rules: {
        required: (value) => !!value || "Required",
      },
    };
  },
  mounted() {
    this.$ajax
      .get("/api/wifi-config")
      .then((data) => {
        this.config = data.data;
      })
      .catch((error) => {
        console.log(error);
      });
  },
  methods: {
    save_config: function () {
      this.$ajax
        .put("/api/wifi-config", this.config)
        .then((data) => {
          console.log(data);
        })
        .catch((error) => {
          console.log(error);
        });
    },
  },
};
</script>
