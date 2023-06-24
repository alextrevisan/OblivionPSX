
-- All of this code is sandboxed, as in any global variable it
-- creates will be attached to a local environment. The global
-- environment is still accessible as normal.
-- Note that the environment is wiped every time the shader is
-- recompiled one way or another.

-- The environment will have the `shaderProgramID` variable set
-- before being evaluated, meaning this code is perfectly valid:
function Constructor(shaderProgramID)
    -- Cache some Shader Attributes locations
    print('Shader compiled: program ID = ' .. shaderProgramID)
end
Constructor(shaderProgramID)

-- This function is called to issue an ImGui::Image when it's time
-- to display the video output of the emulated screen. It can
-- prepare some values to later attach to the shader program.
--
-- This function won't be called for non-ImGui renders, such as
-- the offscreen render of the vram.
function Image(textureID, srcSizeX, srcSizeY, dstSizeX, dstSizeY)
    imgui.Image(textureID, dstSizeX, dstSizeY, 0, 0, 1, 1)
end

-- This function is called to draw some UI, at the same time
-- as the shader editor, but regardless of the status of the
-- shader editor window. Its purpose is to potentially display
-- a piece of UI to let the user interact with the shader program.

-- Returning true from it will cause the environment to be saved.
function Draw()
end

-- This function is called just before executing the shader program,
-- to give it a chance to bind some attributes to it, that'd come
-- from either the global state, or the locally computed attributes
-- from the two functions above.
--
-- The last six parameters will only exist for non-ImGui renders.
function BindAttributes(textureID, shaderProgramID, srcLocX, srcLocY, srcSizeX, srcSizeY, dstSizeX, dstSizeY)
end
