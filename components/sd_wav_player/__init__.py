import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import speaker
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32"]

sd_wav_player_ns = cg.esphome_ns.namespace("sd_wav_player")
SDWavPlayer = sd_wav_player_ns.class_("SDWavPlayer", cg.Component)

CONF_SPEAKER_ID = "speaker_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SDWavPlayer),
        cv.Required(CONF_SPEAKER_ID): cv.use_id(speaker.Speaker),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    speaker_var = await cg.get_variable(config[CONF_SPEAKER_ID])
    cg.add(var.set_speaker(speaker_var))
